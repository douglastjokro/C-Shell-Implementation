#include "pipe.h"
#include <fcntl.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

/* This function redirects the STDOUT of the spcified token to STDOUT
 *Error checks for dup failure and exits program if there is an error.
 */
void reSTDOUT(char *tokenAfter) {
  int new_stdout = open(tokenAfter, O_WRONLY | O_TRUNC | O_CREAT, 0644);
  if (new_stdout < 0) {
    perror("Invalid standard output redirect: No such file or directory");
    exit(EXIT_FAILURE);
    return;
  }
  int dup2Ret = dup2(new_stdout, STDOUT_FILENO);
  if (dup2Ret == -1) {
    perror("Error in redirecting the output.");
    exit(EXIT_FAILURE);
    return;
  }
}

/* This function redirects the STDIN of the spcified token to STDOUT
 *Error checks for dup failure and exits program if there is an error.
 */
void reSTDIN(char *tokenAfter) {
  // open the file in write only mode
  int new_stdin = open(tokenAfter, O_RDONLY, 0644);
  if (new_stdin < 0) {
    perror("Invalid standard input redirect: No such file or directory");
    exit(EXIT_FAILURE);
  }
  int dup2Ret = dup2(new_stdin, STDIN_FILENO);
  if (dup2Ret == -1) {
    perror("Error in redirecting the output.");
    exit(EXIT_FAILURE);
  }
}

/*This functions takes in a double ptr and free it
 *start from the inside later
 */
void freeTwoPointers(char **twoPointers) {
  int i = 0;
  while (twoPointers[i] != NULL) {
    free(twoPointers[i]);
    i++;
  }
  free(twoPointers);
}

/*This function takes in an array of command, and look for "|" symbol(s).
 *Note that in this project, we are only dealing with two-stage pipes.
 *Thus, the case of having two or more "|" symbols in the command array will be
 *determined as invalid input.
 */

int isPipe(char **command) {
  int pipeCount = 0;
  int commandIndex = 0;
  while (command[commandIndex] != NULL) {
    if (strcmp(command[commandIndex], "|") == 0) {
      pipeCount++;
    }
    commandIndex++;
  }
  if (pipeCount == 0) {
    return 0;
  }
  if (pipeCount == 1) {
    return 1;
  }
  return -1;
}

/**This function takes a command array before the piping symbol.
 */
char **createTokenArrayBefore(char **command) {
  // initiate the return array with malloc
  char **returnArray = malloc(sizeof(char *) * 100);
  int commandIndex = 0;
  int returnIndex = 0;
  while (strcmp(command[commandIndex], "|") != 0) {
    returnArray[returnIndex] = malloc(sizeof(char *) * 1024);
    strcpy(returnArray[returnIndex], command[commandIndex]);
    commandIndex++;
    returnIndex++;
  }
  returnArray[returnIndex] = NULL;
  return returnArray;
}

/**This function takes a command array after the piping symbol.
 */
char **createTokenArrayAfter(char **command) {
  char **returnArray = malloc(sizeof(char *) * 100);
  int commandIndex = 0;
  int returnIndex = 0;
  while (strcmp(command[commandIndex], "|") != 0) {
    commandIndex++;
  }
  commandIndex++;
  while (command[commandIndex] != NULL) {
    returnArray[returnIndex] = malloc(sizeof(char) * 1024);
    strcpy(returnArray[returnIndex], command[commandIndex]);
    commandIndex++;
    returnIndex++;
  }
  returnArray[returnIndex] = NULL;
  return returnArray;
}

/*This function takes a command array before the pipe and handles redirection
 */
char **rePipeWrite(char **command1) {
  char **returnArray = NULL;
  int inputIndex = 0;
  int inCount = 0;
  int inIndex = 0;
  while (command1[inputIndex] != NULL) {
    if (strcmp(command1[inputIndex], ">") == 0) {
      perror("Invalid input: piping and redirction conflicts.");
      return NULL;
    }
    if (strcmp(command1[inputIndex], "<") == 0) {
        inCount++;
        if (inCount == 1){inIndex = inputIndex;}
    }
    inputIndex++;
  }
  if (inCount > 1) {
      perror("Invalid input: more than one input redirection found.");
      return NULL;
  }
  returnArray = malloc(sizeof(char*) * 100);
  if (inCount == 0){
      inputIndex = 0;
      int returnIndex = 0;
      while(command1[inputIndex] != NULL){
          returnArray[returnIndex] = malloc(sizeof(char) * 100);
          strcpy(returnArray[returnIndex], command1[inputIndex]);
          inputIndex++;
          returnIndex++;
      }
      returnArray[returnIndex] = NULL;
  }
  if (inCount == 1){
      inputIndex = 0;
      int returnIndex = 0;
      while(command1[inputIndex] != NULL){
          if(inputIndex == inIndex){break;}
          returnArray[returnIndex] = malloc(sizeof(char) * 100);
          strcpy(returnArray[returnIndex], command1[inputIndex]);
          inputIndex++;
          returnIndex++;
      }
      returnArray[returnIndex] = NULL;
      if(command1[inIndex + 1] != NULL){
          reSTDIN(command1[inIndex + 1]);
      }
  }
  if(returnArray[0] == NULL){return NULL;}
  return returnArray;
}

/*This function takes a command array before the pipe and handles redirection.
 */
char **rePipeRead(char **command2) {
  char **returnArray = NULL;
  int inputIndex = 0;
  int outCount = 0;
  int outIndex = 0;
  while (command2[inputIndex] != NULL) {
    if (strcmp(command2[inputIndex], "<") == 0) {
      perror("Invalid input: piping and redirction conflicts.");
      return NULL;
    }
    if (strcmp(command2[inputIndex], ">") == 0) {
        outCount++;
        if (outCount == 1){outIndex = inputIndex;}
    }
    inputIndex++;
  }
  if (outCount > 1) {
      perror("Invalid input: more than one output redirection found.");
      return NULL;
  }
  returnArray = malloc(sizeof(char*) * 100);
  if (outCount == 0){
      inputIndex = 0;
      int returnIndex = 0;
      while(command2[inputIndex] != NULL){
          returnArray[returnIndex] = malloc(sizeof(char) * 100);
          strcpy(returnArray[returnIndex], command2[inputIndex]);
          inputIndex++;
          returnIndex++;
      }
      returnArray[returnIndex] = NULL;
  }
  if (outCount == 1){
      inputIndex = 0;
      int returnIndex = 0;
      while(command2[inputIndex] != NULL){
          if(inputIndex == outIndex){break;}
          returnArray[returnIndex] = malloc(sizeof(char) * 100);
          strcpy(returnArray[returnIndex], command2[inputIndex]);
          inputIndex++;
          returnIndex++;
      }
      returnArray[returnIndex] = NULL;
      if(command2[outIndex + 1] != NULL){
          reSTDOUT(command2[outIndex + 1]);
      }
  }
  if(returnArray[0] == NULL){return NULL;}
  return returnArray;
}
