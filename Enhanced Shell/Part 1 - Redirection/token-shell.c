#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <fcntl.h>
#include "tokenizer.h"

#define INPUT_SIZE 1024

pid_t childPid = 0;

void killChildProcess();

void sigintHandler(int sig);

void registerSignalHandlers();

void freeTwoPointers(char **twoPointers);

void reSTDIN(char* nextToken);

void reSTDOUT( char *nextToken);

void executeShell();

void writeToStdout(char *text);

char **getCommandFromInput();

int main(int argc, char **argv) {
    registerSignalHandlers();
    while (1) {
        executeShell();
    }
    return 0;
}

/* Sends SIGKILL signal to a child process.
 * Error checks for kill system call failure and exits program if
 * there is an error */
void killChildProcess() {
    if (kill(childPid, SIGKILL) == -1) {
        perror("Error in kill");
        exit(EXIT_FAILURE);
    }
}

/* Signal handler for SIGALRM. Catches SIGALRM signal and
 * kills the child process if it exists and is still executing.
 * It then prints out penn-shredder's catchphrase to standard output */
void alarmHandler(int sig) {
    if (sig == SIGALRM){
        if (childPid != 0){
            killChildProcess();
            childPid = 0;
            writeToStdout("Bwahaha ... tonight I dine on turtle soup");
        }
    }
}

/* Signal handler for SIGINT. Catches SIGINT signal (e.g. Ctrl + C) and
 * kills the child process if it exists and is executing. Does not
 * do anything to the parent process and its execution */
void sigintHandler(int sig) {
    if (childPid != 0) {
        killChildProcess();
    }
}

/* Registers SIGALRM and SIGINT handlers with corresponding functions.
 * Error checks for signal system call failure and exits program if
 * there is an error */
void registerSignalHandlers() {
    #ifdef DEBUG
        fprintf(stderr, "pid = %d: at line %d in function %s\n", childPid, __LINE__, __func__);
    #endif

    if (signal(SIGINT, sigintHandler) == SIG_ERR) {
        perror("Error in signal");
        exit(EXIT_FAILURE);
    }

    if (signal(SIGALRM, alarmHandler) == SIG_ERR){
        perror("Error in signal");
        exit(EXIT_FAILURE);
    }
}

/*This functions accepts and frees a double pointer **.
 */
void freeTwoPointers(char **twoPointers){
    int i = 0;
    while (twoPointers[i] != NULL){
        free(twoPointers[i]);
        i++;
    }
    free(twoPointers);
}

/* This function redirects the STDIN to STDOUT of a specified token
 */
void reSTDIN( char *nextToken){
    int new_stdin = open(nextToken, O_RDONLY, 0644);
    if(new_stdin < 0){
        perror("Invalid standard input redirect: No such file or directory");
        exit(EXIT_FAILURE);
    }
    int dup2Ret = dup2(new_stdin,STDIN_FILENO);
    if(dup2Ret == -1){
        perror("Error in redirecting the output.");
        exit(EXIT_FAILURE);
    }
}

/*  This function redirects the STDOUT to STDOUT of a specified token.
 */
void reSTDOUT( char *nextToken){
    int new_stdout = open(nextToken, O_WRONLY | O_TRUNC | O_CREAT, 0644);
    if(new_stdout < 0){
        perror("Invalid standard output redirect: No such file or directory");
        exit(EXIT_FAILURE);
    }
    int dup2Ret = dup2(new_stdout,STDOUT_FILENO);
    if(dup2Ret == -1){
        perror("Error in redirecting the output.");
        exit(EXIT_FAILURE);
    }
}

/* Prints the shell prompt and waits for input from user.
 * Takes timeout as an argument and starts an alarm of that timeout period
 * if there is a valid command. It then creates a child process which
 * executes the command with its arguments.
 *
 * The parent process waits for the child. On unsuccessful completion,
 * it exits the shell. */
void executeShell() {
    char **command;
    int status;
    char minishell[] = "penn-sh> ";
    writeToStdout(minishell);
    command = getCommandFromInput();
    if (command != NULL) {
        childPid = fork();
        if (childPid < 0) {
            freeTwoPointers(command);
            perror("Error in creating child process");
            exit(EXIT_FAILURE);
        }
        if (childPid == 0) {
            int commandIndex = 0;
            int argumentIndex = 0;
            int inCount = 0;
            int outCount = 0;
            char *args [100] = {NULL};
            while(command[commandIndex] != NULL){
                if (strcmp(command[commandIndex], ">") == 0){
                    outCount++;
                    if (outCount > 1){
                        perror("Invalid: Multiple standard output redirects");
                        exit(EXIT_FAILURE);
                    }
                    commandIndex++;
                    reSTDOUT(command[commandIndex]);
                    commandIndex++;
                    continue;
                }
                if (strcmp(command[commandIndex], "<") == 0){
                    inCount++;
                    if (inCount > 1){
                        perror("Invalid: Multiple standard input redirects");
                        exit(EXIT_FAILURE);
                    }
                    commandIndex++;
                    reSTDIN(command[commandIndex]);
                    commandIndex++;
                    continue;
                }
                args[argumentIndex] = command[commandIndex];
                argumentIndex++;
                commandIndex++;
            }
            args[argumentIndex] = NULL;
            if(execvp(args[0], args) == -1){
                freeTwoPointers(command);
                perror("Error in creating child process");
                exit(EXIT_FAILURE);
            }
        } else {
            do {
                if (wait(&status) == -1) {
 
                    freeTwoPointers(command);
                    perror("Error in child process termination");
                    exit(EXIT_FAILURE);
                }
            } while (!WIFEXITED(status) && !WIFSIGNALED(status));
            childPid = 0;
        }
        freeTwoPointers(command);
    }
}

/* Writes particular text to standard output */
void writeToStdout(char *text) {
    if (write(STDOUT_FILENO, text, strlen(text)) == -1) {
        perror("Error in write");
        exit(EXIT_FAILURE);
    }
}

/* Reads input from standard input till it reaches a new line character.
 * Checks if EOF (Ctrl + D) is being read and exits penn-shredder if that is the case
 * Otherwise, it checks for a valid input and adds the characters to an input buffer.
 *
 * From this input buffer, the first 1023 characters (if more than 1023) or the whole
 * buffer are assigned to command and returned. An \0 is appended to the command so
 * that it is null terminated
 * after getting the command, it will break the command into tokens, and store it into
 * an array of null terminated strings. */
char **getCommandFromInput() {
    char *input = malloc(sizeof(char)*INPUT_SIZE);
    char *command;
    char** returnArray;
    if(input == NULL){
        perror("Error in creating reading buffer.");
        exit(EXIT_FAILURE);
    }
    ssize_t readIn = read(STDIN_FILENO, input, INPUT_SIZE-1);
    if(readIn == -1){
        free(input);
        perror("Error in read.");
        exit(EXIT_FAILURE);
    }
    if(readIn == 0){
        free(input);
        exit(EXIT_SUCCESS);
    }
    int leadingSpace = 0;
    for(int i = 0; i < readIn;i++){
        if(input[i] != ' '){break;}
        leadingSpace++;
    }
    if((input[readIn-1] == '\n') && (readIn - 1 == leadingSpace)){free(input); return NULL;}
    int trailingSpace = 0;
    for(int i = readIn-1;i>=0;i--){
        if((input[i] != ' ') && (input[i] != '\n')){break;}
        trailingSpace++;
    }
    command = malloc(sizeof(char) * (readIn + 1));
    int j = 0;
    for(int i = leadingSpace;i < (readIn - trailingSpace);i++){
        command[j] = input[i];
        j++;
    }
    command[j] = '\0';
    free(input);
    returnArray = malloc(sizeof(char*) * INPUT_SIZE);
    char **temp = returnArray;
    TOKENIZER *tokenizer = init_tokenizer(command);
    if(tokenizer != NULL){
        while(1) {
            char* tokenizedString = get_next_token(tokenizer);
            *temp = tokenizedString;
            if (tokenizedString == NULL){
                break;
            }
            temp++;
        }
    free_tokenizer(tokenizer);
    free(command);
    return returnArray;
    }
    return NULL;
}
