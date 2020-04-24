#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/wait.h>

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
int historyCommand();

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

void execute(char** args) {
    pid_t pid;
    int status;

    if (runInBackground) {
        if ((pid = fork()) == 0) {
            setpgid(0, 0);
            execvp(*args, args);
        }
        else {
            printf("%d\n", pid);
            return;
        }
    }
    else {
        if ((pid = fork()) == 0) {
            if (execvp(*args, args) == -1) {
                fprintf(stderr, ERROR_MESSAGE);
            }
        } else {
            printf("%d\n", pid);
            if (waitpid(pid, NULL, 0) != pid) {
                printf("Error\n");
            }
        }
    }
}

int main(void) {
    char line[MAX_CMND_LENGTH];
    char** args = NULL;
    getcwd(workingDir, 100);

    while(1) {
        runInBackground = false;
        printf("%s", "> ");
        fgets(line, MAX_CMND_LENGTH, stdin);

        for (int i = 0;line[i] != '\0';i++)
        {
            if (line[i] == ' ' && line[i+1] != ' ')
                count++;
        }
        count++;

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

        args = (char**)malloc(count * sizeof(char*));

        int code = parse(line, args);
        if (code == cdCode || code == 0) {
            count = 0;
            free(args);
            continue;
        }

        count = 0;
        execute(args);
        free(args);
    }
}