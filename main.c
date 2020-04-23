#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/wait.h>

#define MAX_CMND_LENGTH 100
#define ERROR_MESSAGE "Error in system call\n"
#define cdCode 2
#define builtinCode 1
typedef enum { false, true } bool;

bool runInBackground = false;
int count = 0;
char workingDir[100];

int parse(char* line, char** args) {
    char* token;
    line = strtok(line, "\n");
    token = strtok(line, " ");

    if (strcmp(token, "exit") == 0) {
        exit(1);
    }
    else if (strcmp(token, "cd") == 0) {
        if (count > 2) {
            fprintf(stderr, ERROR_MESSAGE);
        }
        else if (count == 2) {
            token = strtok(NULL, " ");
            printf("%s\n", token);
            int outCode = chdir(token);
            if (outCode == -1) {
                fprintf(stderr, ERROR_MESSAGE);
            }
        }
        else {
            chdir(workingDir);
        }

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

void execute(char** args) {
    pid_t pid;
    int status;

    if ((pid = fork()) == 0) {
        execvp(*args, args);
    } else {
        printf("%d\n", getpid());
        if (!runInBackground) {
            wait(&status);
        }
    }
}

int main(void) {
    char line[MAX_CMND_LENGTH];
    char** args = NULL;
    getcwd(workingDir, 100);

    while(1) {
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

        args = (char**)malloc((count + 1) * sizeof(char*));

        int code = parse(line, args);
        if (code == cdCode) {
            count = 0;
            free(args);
            continue;
        }

        execute(args);
        free(args);

        count = 0;
    }
}