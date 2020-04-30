#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <sys/types.h>

/*/
 * Macros we will use
 */
#define MAX_CMND_LENGTH 100 // Max command length
#define ERROR_MESSAGE "Error in system call\n"
#define ERROR_CD_FILE "No such file or directory\n"
#define ERROR_CD_ARG "Too many arguments\n"
#define NoExecCode 2 // Exit code for non-built-in commands
#define builtinCode 1 // Exit code for built in commands
typedef enum { false, true } bool;

/*
 * Globals we will use
 */
bool runInBackground = false;
int count = 0; // count the number of words in command
char workingDir[100] = "";
char* homedir = NULL;

/*
 * Functions
 */
int cdCommand(char* line, char* token);
void execute(char* line, char** args);
void historyCommand();
void jobsCommand();

/*
 * A job struct, a jobs array of 100 jobs
 */
struct job {
    char command[MAX_CMND_LENGTH] ;
    pid_t pid;
    char status[MAX_CMND_LENGTH] ;
} jobs[MAX_CMND_LENGTH];

int jobIndex = 0; // represents the current size of actual job array

/*
 * Parse the command, save into args array
 */
int parse(char* line, char** args) {
    char* token;
    line = strtok(line, "\n"); // parse according to "\n"
    char copyOfLine[strlen(line)];
    strcpy(copyOfLine, line);
    token = strtok(line, " "); // parse according to " "

    if (strcmp(token, "exit") == 0) {
        printf("%d\n", getpid());
        exit(0);
    }
    else if (strcmp(token, "cd") == 0) {
        token = strtok(NULL, " ");
        return cdCommand(copyOfLine, token);
    }
    else if (strcmp(token, "history") == 0) {
        historyCommand();
        return NoExecCode;
    }
    else if (strcmp(token, "jobs") == 0) {
        jobsCommand();
        return NoExecCode;
    }
    else if (strcmp(token, "clear") == 0) {
        write(1, "\33[H\33[2J", 7);
        return NoExecCode;
    }

    /*
     * continue parsing with respect to spaces,
     * insert into args array. If encountering "&",
     * stop and set runInBackground true.
     */
    while (token != NULL) {
        if (strcmp(token, "&") == 0) {
            runInBackground = true;
            break;
        }
        *args++ = token;
        token = strtok(NULL, " ");
    }

    *args = NULL; // Make final element NULL for execvp
    return builtinCode;
}

/*
 * Insert a new job into array, with it's command and pid
 */
void insertJob(char* line, pid_t pid_in) {
    strcpy(jobs[jobIndex].command, line);
    jobs[jobIndex].pid = pid_in;
    strcpy(jobs[jobIndex].status, "DONE");

    jobIndex++;
}

/*
 * cd command, token are the arguments
 */
int cdCommand(char* line, char* token) {
    insertJob(line, getpid());

    if (count > 2) {
        printf("%d\n", getpid());
        fprintf(stderr, ERROR_CD_ARG);
    }
    else if (count == 2) {
        printf("%d\n", getpid());
        if (strcmp(token, "~") == 0) {
            getcwd(workingDir, 100);
            if (chdir(homedir) == -1) {
              fprintf(stderr, ERROR_MESSAGE);
            }
            return NoExecCode;
        }
        else if (strcmp(token, "-") == 0) {
            if (strcmp(workingDir, "") == 0) {
            return NoExecCode;
            }
            if (chdir(workingDir) == -1) {
              fprintf(stderr, ERROR_MESSAGE);
            }
            return NoExecCode;
        }

        getcwd(workingDir, 100);
        int outCode = chdir(token);
        if (outCode == -1) {
            fprintf(stderr, ERROR_CD_FILE);
        }
        return NoExecCode;
    }
    else {
        printf("%d\n", getpid());
        getcwd(workingDir, 100);
        if (chdir(homedir) == -1) {
              fprintf(stderr, ERROR_MESSAGE);
            }
    }

    return NoExecCode;
}

/*
 * List the history of the commands
 */
void historyCommand() {
    int status;
    int wait;
    
    int i;
    int j;
    /*
     * iterate jobs array, set status according to the status of the jobs
     */
    for (i = 0; i <= jobIndex - 1; i++) {
        wait = waitpid(jobs[i].pid, &status, WNOHANG);
        if (wait == 0) {
            strcpy(jobs[i].status, "RUNNING");
        } else {
            strcpy(jobs[i].status, "DONE");
        }
    }

    /*
     * iterate jobs array, print info of each job
     */
    for (j = 0; j <= jobIndex - 1; j++) {
        printf("%d ", jobs[j].pid);
        printf("%s ", jobs[j].command);
        printf("%s\n", jobs[j].status);
    }

    // Manually print history running
    printf("%d ", getpid());
    printf("%s ", "history");
    printf("%s\n", "RUNNING");

    // insert history into jobs array for future reference
    insertJob("history", getpid());
}

/*
 * List running jobs
 */
void jobsCommand() {
    int status;
    int wait;
    
    int i;
    int j;
    /*
     * iterate jobs array, set status according to the status of the jobs
     */
    for (i = 0; i <= jobIndex - 1; i++) {
        wait = waitpid(jobs[i].pid, &status, WNOHANG);
        if (wait == 0) {
            strcpy(jobs[i].status, "RUNNING");
        } else {
            strcpy(jobs[i].status, "DONE");
        }
    }
    /*
     * iterate jobs array and print info about running jobs
     */
    for (j = 0; j <= jobIndex - 1; j++) {
        if (strcmp(jobs[j].status, "RUNNING") == 0) {
            printf("%d ", jobs[j].pid);
            printf("%s\n", jobs[j].command);
        }
    }
    
    // insert jobs into jobs array for future reference
    insertJob("jobs", getpid());
}

/*
 * execute commands via exec syscall
 */
void execute(char* line, char** args) {
    pid_t pid;
    int status;
    int waited;

    pid = fork(); // create child thread
    if (pid < 0) {
        fprintf(stderr, ERROR_MESSAGE);
        exit(1);
    }

    if (runInBackground) { // background command
        if (pid == 0) {
            if (execvp(*args, args) == -1) {
                fprintf(stderr, ERROR_MESSAGE);
            }
            exit(0);
        } else {
            printf("%d\n", pid);
            insertJob(line, pid);
        }
    } else { // foreground command
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

/*
 * remove quotes from a string, ex: "hi" -> hi
 */
void removeQuotes(char* line) {
    int j = 0;
    int i = 0;
    for (i = 0; i < strlen(line); i ++) {
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

/*
 * count words in string
 */
void countWord(char* line) {
    int i = 0;
    
    for (i = 0; line[i] != '\0'; i++)
    {
        if (line[i] == ' ' && line[i+1] != ' ')
            count++;
    }
    count++;
}

/*
 * reset globals in the code
 */
void resetGlobals() {
    count = 0;
    runInBackground = false;
}

int main(void) {
    char line[MAX_CMND_LENGTH];
    char** args = NULL;
    homedir = getenv("HOME");

    /*
     * our main loop
     */
    while(1) {
        printf("%s", "> "); // print prompt
        fgets(line, MAX_CMND_LENGTH, stdin); // get command from user

        countWord(line); // count words
        removeQuotes(line); // remove quotes

        args = (char**)malloc(count * sizeof(char*));

        int code = parse(line, args); // parse the command
        /*
         * if the command doesn't need the execute command, continue
         */
        if (code == NoExecCode || code == 0) {
            resetGlobals();
            free(args);
            continue;
        }

        /*
         * execute command, and continue
         */
        execute(line, args);
        resetGlobals();
        free(args);
    }
}
