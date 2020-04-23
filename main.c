#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/wait.h>

#define MAX_CMND_LENGTH 100
#define ERROR_MESSAGE "Error in system call"
typedef enum { false, true } bool;

bool runInBackground = false;

void parse(char* line, char** args) {
    char* token;
    line = strtok(line, "\n");
    token = strtok(line, " ");

    while (token != NULL) {
        if (strcmp(token, "&") == 0) {
            runInBackground = true;
            token = strtok(NULL, " ");
            continue;
        }
        *args++ = token;
        token = strtok(NULL, " ");
    }

    *args = NULL;
}

void execute(char** args) {
    pid_t pid;
    int status;

    if ((pid = fork()) < 0) {
        fprintf(stderr, ERROR_MESSAGE);
        exit(1);
    }
    else if (pid == 0) {
        printf("%d\n", getpid());
        if (execvp(*args, args) < 0) {
            fprintf(stderr, ERROR_MESSAGE);
            exit(1);
        }
    }
    else {
        while (wait(&status) != pid) {}
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