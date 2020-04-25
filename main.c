#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <errno.h>
#include <signal.h>

#define MAX_CMND_LENGTH 100
#define ERROR_MESSAGE "Error in system call\n"
#define ERROR_CD_FILE "No such file or directory\n"
#define ERROR_CD_ARG "Too many arguments\n"
#define cdCode 2
#define builtinCode 1
typedef enum { false, true } bool;

bool runInBackground = false;
bool historyCmd = false;
int count = 0;
char workingDir[100];
int cdCommand(char* token);
void execute(char* line, char** args);

struct job {
    char* command;
    pid_t pid;
    int status;
} jobs[MAX_CMND_LENGTH] ;

int jobIndex = 0;

void updateStatus() {
    for (int i = 0; i < jobIndex - 1; i++) {
        int status = kill(jobs[i].pid, 0);
        if (status == 0) {
            jobs[i].status = 1;
        } else {
            jobs[i].status = 0;
        }
    }
};

int parse(char* line, char** args) {
    char* token;
    line = strtok(line, "\n");
    token = strtok(line, " ");

    if (strcmp(token, "exit") == 0) {
        printf("%d\n", getpid());
        exit(1);
    }
    else if (strcmp(token, "cd") == 0) {
        token = strtok(NULL, " ");
        return cdCommand(token);
    }
    else if (strcmp(token, "history") == 0) {
        historyCmd = true;
    }
    else if (strcmp(token, "clear") == 0) {
        write(1, "\33[H\33[2J", 7);
        return 0;
    }

    while (token != NULL) {
        if (strcmp(token, "&") == 0) {
            runInBackground = true;
            break;
        }
        *args++ = token;
        token = strtok(NULL, " ");
    }

    *args = NULL;
    return builtinCode;
}

void insertJob(char* line, pid_t pid) {
    jobs[jobIndex].command = (char *) malloc(strlen(line) * sizeof(char));
    strcpy(jobs[jobIndex].command, line);
    jobs[jobIndex].pid = pid;
    jobs[jobIndex].status = 1;
    jobIndex++;

    updateStatus();
}

int cdCommand(char* token) {
    if (count > 2) {
        printf("%d\n", getpid());
        fprintf(stderr, ERROR_CD_ARG);


    }
    else if (count == 2) {
        printf("%d\n", getpid());
        int outCode = chdir(token);
        if (outCode == -1) {
            printf("%d\n", getpid());
            fprintf(stderr, ERROR_CD_FILE);
        }
    }
    else {
        printf("%d\n", getpid());
        chdir(workingDir);
    }

    return cdCode;
}

void execute(char* line, char** args) {
    pid_t pid;
    int status;

    if (historyCmd) {
        if ((pid = fork()) == 0) {
            kill(getpid(), SIGINT);
        } else {
            insertJob("history", pid);

            for (int i = 0; i <= jobIndex - 1; i++) {
                printf("%d ", jobs[i].pid);
                printf("%s ", jobs[i].command);
                printf("%d\n", jobs[i].status);
            }

            if (waitpid(pid, NULL, 0) != pid) {
                printf("Error\n");
            }
        }
    }
    else {
        if (runInBackground) {
            if ((pid = fork()) == 0) {
                execvp(*args, args);
            } else {
                insertJob(line, pid);
                printf("%d\n", pid);
            }
        } else {
            if ((pid = fork()) == 0) {
                if (execvp(*args, args) == -1) {
                    fprintf(stderr, ERROR_MESSAGE);
                }
                exit(0);
            } else {
                insertJob(line, pid);
                printf("%d\n", pid);
                if (waitpid(pid, NULL, 0) != pid) {
                    printf("Error\n");
                }
            }
        }
    }
}

void removeQuotes(char* line) {
    int j = 0;
    for (int i = 0; i < strlen(line); i ++) {
        if (line[i] != '"' && line[i] != '\\') {
            line[j++] = line[i];
        } else if (line[i+1] == '"' && line[i] == '\\') {
            line[j++] = '"';
        } else if (line[i+1] != '"' && line[i] == '\\') {
            line[j++] = '\\';
        }
    }

    if(j>0) line[j]=0;
}

void countWord(char* line) {
    for (int i = 0; line[i] != '\0'; i++)
    {
        if (line[i] == ' ' && line[i+1] != ' ')
            count++;
    }
    count++;
}

void resetGlobals() {
    count = 0;
    historyCmd = false;
    runInBackground = false;
}

int main(void) {
    char line[MAX_CMND_LENGTH];
    char copy[MAX_CMND_LENGTH];
    char** args = NULL;
    getcwd(workingDir, 100);

    while(1) {
        printf("%s", "> ");
        fgets(line, MAX_CMND_LENGTH, stdin);
        strcpy(copy, line);

        countWord(line);
        removeQuotes(line);

        args = (char**)malloc(count * sizeof(char*));

        int code = parse(line, args);
        if (code == cdCode || code == 0) {
            resetGlobals();
            free(args);
            continue;
        }

        execute(line, args);
        resetGlobals();
        free(args);
    }
}