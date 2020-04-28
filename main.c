#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <signal.h>

#define MAX_CMND_LENGTH 100
#define ERROR_MESSAGE "Error in system call\n"
#define ERROR_CD_FILE "No such file or directory\n"
#define ERROR_CD_ARG "Too many arguments\n"
#define cdCode 2
#define builtinCode 1
typedef enum { false, true } bool;

bool runInBackground = false;
int count = 0;
char workingDir[100];
int cdCommand(char* token);
void execute(char* line, char** args);
void historyCommand();
void jobsCommand();

struct job {
    char* command;
    pid_t pid;
    char* status;
} jobs[MAX_CMND_LENGTH];
int jobIndex = 0;

void printcommand() {
    int status;
    int wait;
    for (int i = 0; i <= jobIndex - 1; i++) {
        wait = waitpid(jobs[i].pid, &status, WNOHANG);
        printf("job %s: %d\n",jobs[i].command, wait);
    }
}

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
        historyCommand();
        return cdCode;
    }
    else if (strcmp(token, "jobs") == 0) {
        jobsCommand();
        return cdCode;
    }
    else if (strcmp(token, "clear") == 0) {
        write(1, "\33[H\33[2J", 7);
        return 0;
    }
    else if (strcmp(token, "print") == 0) {
        printcommand();
        return cdCode;
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
    jobs[jobIndex].status = (char*)malloc(strlen("DONE") * sizeof(char));
    strcpy(jobs[jobIndex].status, "DONE");
    jobIndex++;
}

int cdCommand(char* token) {
    insertJob("cd", getpid());

    if (count > 2) {
        printf("%d\n", getpid());
        fprintf(stderr, ERROR_CD_ARG);
    }
    else if (count == 2) {
        printf("%d\n", getpid());
        if (strcmp(token, "~") == 0) {
            printf("%d\n", getpid());
            chdir(workingDir);
            return cdCode;
        }

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

void historyCommand() {
    int status;
    int wait;
    for (int i = 0; i <= jobIndex - 1; i++) {
        wait = waitpid(jobs[i].pid, &status, WNOHANG);
        if (wait == 0) {
            jobs[i].status = (char *) realloc(jobs[i].status, strlen("RUNNING") * sizeof(char));
            strcpy(jobs[i].status, "RUNNING");
        } else {
            jobs[i].status = (char *) realloc(jobs[i].status, strlen("DONE") * sizeof(char));
            strcpy(jobs[i].status, "DONE");
        }
    }

    for (int i = 0; i <= jobIndex - 1; i++) {
        printf("%d ", jobs[i].pid);
        printf("%s ", jobs[i].command);
        printf("%s\n", jobs[i].status);
    }

    printf("%d ", getpid());
    printf("%s ", "history");
    printf("%s\n", "RUNNING");

    insertJob("history", getpid());
}

void jobsCommand() {
    int status;
    int wait;

    for (int i = 0; i <= jobIndex - 1; i++) {
        wait = waitpid(jobs[i].pid, &status, WNOHANG);
        if (wait == 0) {
            jobs[i].status = (char *) realloc(jobs[i].status, strlen("RUNNING") * sizeof(char));
            strcpy(jobs[i].status, "RUNNING");
        } else {
            jobs[i].status = (char *) realloc(jobs[i].status, strlen("DONE") * sizeof(char));
            strcpy(jobs[i].status, "DONE");
        }
    }

    for (int i = 0; i <= jobIndex - 1; i++) {
        if (strcmp(jobs[i].status, "RUNNING") == 0) {
            printf("%d ", jobs[i].pid);
            printf("%s\n", jobs[i].command);
        }
    }
}

void execute(char* line, char** args) {
    pid_t pid;
    int status;
    int waited;

    pid = fork();

    if (runInBackground) {
        if (pid == 0) {
            execvp(*args, args);
            exit(0);
        } else {
            printf("%d\n", getpid());
            insertJob(line, pid);
        }
    } else {
        if (pid == 0) {
            printf("%d\n", getpid());
            if (execvp(*args, args) == -1) {
                fprintf(stderr, ERROR_MESSAGE);
            }
            exit(0);
        } else {
            do {
                waited = waitpid(pid, &status, WNOHANG);
            } while (waited != pid);
            insertJob(line, pid);
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