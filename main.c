#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/wait.h>

#define MAX_CMND_LENGTH 100
#define ERROR_MESSAGE "Error in system call\n"
typedef enum { false, true } bool;

bool runInBackground = false;

void parse(char* line, char** args) {
    char* token;
    line = strtok(line, "\n");
    token = strtok(line, " ");

    if (strcmp(token, "exit") == 0) {
        exit(1);
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
    int count = 0;
    while(1) {
        printf("%s", "> ");
        fgets(line, MAX_CMND_LENGTH, stdin);

        for (int i = 0;line[i] != '\0';i++)
        {
            if (line[i] == ' ' && line[i+1] != ' ')
                count++;
        }

        args = (char**)malloc((count + 1) * sizeof(char*));
        parse(line, args);

        execute(args);
        free(args);
    }
}