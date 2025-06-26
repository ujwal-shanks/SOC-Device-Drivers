#include <stdio.h>
#include <sys/types.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <signal.h>

#define MAX_INPUT_SIZE 1024
#define MAX_TOKEN_SIZE 64
#define MAX_NUM_TOKENS 64

int fore_gpid = -1;
int kill_all = 0;

void sigint_handler(int sig) {
    printf("\n");
}

void clean(char **tokens, int size) {
    for (int i = 0; i < size; i++) {
        free(tokens[i]);
    }
    free(tokens);
}

char **tokenize(char *line) {
    char **tokens = (char **)malloc(MAX_NUM_TOKENS * sizeof(char *));
    char *token = (char *)malloc(MAX_TOKEN_SIZE * sizeof(char));
    int i, tokenIndex = 0, tokenNo = 0;

    for (i = 0; i < strlen(line); i++) {
        char readChar = line[i];
        if (readChar == ' ' || readChar == '\n' || readChar == '\t') {
            token[tokenIndex] = '\0';
            if (tokenIndex != 0) {
                tokens[tokenNo] = (char *)malloc(MAX_TOKEN_SIZE * sizeof(char));
                strcpy(tokens[tokenNo++], token);
                tokenIndex = 0;
            }
        } else {
            token[tokenIndex++] = readChar;
        }
    }

    free(token);
    tokens[tokenNo] = NULL;
    return tokens;
}

void series(char **tokens, int size) {
    int start = 0;
    for (int i = 0; i <= size; i++) {
        if (i == size || strcmp(tokens[i], "&&") == 0) {
            tokens[i] = NULL;
            int ret = fork();
            if (ret == 0) {
                execvp(tokens[start], tokens + start);
                perror("exec failed");
                exit(1);
            } else if (ret > 0) {
                if (start == 0) fore_gpid = ret;
                setpgid(ret, fore_gpid);
                int status;
                waitpid(ret, &status, 0);
                if (WIFEXITED(status) && WEXITSTATUS(status) != 0) {
                    printf("EXITSTATUS: %d\n", WEXITSTATUS(status));
                }
                start = i + 1;
            }
        }
    }
}

void parallel(char **tokens, int size) {
    int start = 0;
    for (int i = 0; i <= size; i++) {
        if (i == size || strcmp(tokens[i], "&&&") == 0) {
            tokens[i] = NULL;
            int ret = fork();
            if (ret == 0) {
                execvp(tokens[start], tokens + start);
                perror("exec failed");
                exit(1);
            } else if (ret > 0) {
                if (start == 0) fore_gpid = ret;
                setpgid(ret, fore_gpid);
                start = i + 1;
            }
        }
    }

    for (int i = 0; i < 64; i++) {
        int status;
        if (waitpid(-1, &status, 0) > 0 && WIFEXITED(status) && WEXITSTATUS(status) != 0) {
            printf("EXITSTATUS: %d\n", WEXITSTATUS(status));
        }
    }
}

int main(int argc, char *argv[]) {
    int back_procs[64];
    for (int i = 0; i < 64; i++) back_procs[i] = -1;

    char line[MAX_INPUT_SIZE];
    char **tokens;
    int i;

    signal(SIGINT, sigint_handler);

    while (1) {
        for (int i = 0; i < 64; i++) {
            if (back_procs[i] != -1) {
                waitpid(back_procs[i], NULL, WNOHANG);
                back_procs[i] = -1;
            }
        }

        kill_all = 0;
        bzero(line, sizeof(line));

        char cwd[256];
        getcwd(cwd, sizeof(cwd));
        printf("%s $ ", cwd);

        if (fgets(line, sizeof(line), stdin) == NULL) {
            printf("\n");
            continue;
        }

        if (line[0] == '\n') continue;

        tokens = tokenize(line);
        int size = 0, and2 = 0, and3 = 0;

        while (tokens[size] != NULL) {
            if (strcmp(tokens[size], "&&") == 0) and2 = 1;
            if (strcmp(tokens[size], "&&&") == 0) and3 = 1;
            size++;
        }

        if (and2 && and3) {
            fprintf(stderr, "Cannot combine && and &&&\n");
            clean(tokens, size);
            continue;
        }

        if (and2) {
            series(tokens, size);
            clean(tokens, size);
            continue;
        }

        if (and3) {
            parallel(tokens, size);
            clean(tokens, size);
            continue;
        }

        if (size == 1 && strcmp(tokens[0], "exit") == 0) {
            for (int i = 0; i < 64; i++) {
                if (back_procs[i] != -1) {
                    kill(back_procs[i], SIGTERM);
                    waitpid(back_procs[i], NULL, 0);
                }
            }
            clean(tokens, size);
            break;
        }

        if (strcmp(tokens[size - 1], "&") == 0) {
            int ret = fork();
            if (ret == 0) {
                tokens[size - 1] = NULL;
                execvp(tokens[0], tokens);
                perror("exec failed");
                exit(1);
            } else {
                for (int i = 0; i < 64; i++) {
                    if (back_procs[i] == -1) {
                        back_procs[i] = ret;
                        break;
                    }
                }
            }
        } else {
            int ret = fork();
            if (ret == 0) {
                execvp(tokens[0], tokens);
                perror("exec failed");
                exit(1);
            } else if (ret > 0) {
                int status;
                if (waitpid(ret, &status, 0) > 0 && WIFEXITED(status) && WEXITSTATUS(status) != 0) {
                    printf("EXITSTATUS: %d\n", WEXITSTATUS(status));
                }
            }
        }

        clean(tokens, size);
    }

    return 0;
}
