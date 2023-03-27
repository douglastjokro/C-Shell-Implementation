#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <fcntl.h>
#include "tokenizer.h"
#include "pipe.h"

#define INPUT_SIZE 1024

pid_t childPid = 0;

pid_t childPid1 = 0;

pid_t childPid2 = 0;

void killChildProcess();

void sigintHandler(int sig);

void registerSignalHandlers();

void execute2a(char **command);

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

  if (sig == SIGALRM) {
    if (childPid != 0) {
      killChildProcess(); // kill the child process
      childPid = 0;       // set the childPid back to 0
      writeToStdout("Bwahaha ... tonight I dine on turtle soup");
    }
  }
}

/* Signal handler for SIGINT. Catches SIGINT signal (e.g. Ctrl + C) and
 * kills the child process if it exists and is executing. Does not
 * do anything to the parent process and its execution */
void sigintHandler(int sig) {

  // in parent process
  if (childPid != 0) {
    killChildProcess(); // kill the child process
    // childPid = 0; //set the childPid back to 0
  }
}

/* Registers SIGALRM and SIGINT handlers with corresponding functions.
 * Error checks for signal system call failure and exits program if
 * there is an error
 */
void registerSignalHandlers() {
#ifdef DEBUG
  fprintf(stderr, "pid = %d: at line %d in function %s\n", childPid, __LINE__,
          __func__);
#endif

  if (signal(SIGINT, sigintHandler) == SIG_ERR) {
    perror("Error in signal");
    exit(EXIT_FAILURE);
  }

  if (signal(SIGALRM, alarmHandler) == SIG_ERR) {
    perror("Error in signal");
    exit(EXIT_FAILURE);
  }
}

/*
This function executes the shell as directed by project2a (no piping)
*/
void execute2a(char **command) {
  int status;
  int childPid = fork();
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
    char *args[100] = {NULL};
    while (command[commandIndex] != NULL) {
      if (strcmp(command[commandIndex], ">") == 0) {
        outCount++;
        if (outCount > 1) {
          perror("Invalid: Multiple standard output redirects");
          freeTwoPointers(command);
          exit(EXIT_FAILURE);
        }
        commandIndex++;
        reSTDOUT(command[commandIndex]);
        commandIndex++;
        continue;
      }
      if (strcmp(command[commandIndex], "<") == 0) {
        inCount++;
        if (inCount > 1) {
          perror("Invalid: Multiple standard input redirects");
          freeTwoPointers(command);
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
    if (execvp(args[0], args) == -1) {
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

/* Prints the shell prompt and waits for input from user.
 * Takes timeout as an argument and starts an alarm of that timeout period
 * if there is a valid command. It then creates a child process which
 * executes the command with its arguments.
 *
 * The parent process waits for the child. On unsuccessful completion,
 * it exits the shell. */
void executeShell() {
  char **command;
  char minishell[] = "penn-sh> ";
  int piping = 0;
  writeToStdout(minishell);
    command = getCommandFromInput();
  if (command != NULL) {
    piping = isPipe(command);
    if (piping < 0) {
      perror("Invalid: more than two staged pipes.");
      freeTwoPointers(command);
      exit(EXIT_FAILURE);
    }
    if (piping == 0) {
      execute2a(command);
    }
    if (piping == 1) {
      int status1;
      int status2;
      char **command1;
      char **command2;
      command1 = createTokenArrayBefore(command);
      command2 = createTokenArrayAfter(command);
      freeTwoPointers(command);
      int fd[2];
      int pipeCall = pipe(fd);
      if (pipeCall < 0) {
        perror("Error in pipe call.");
        freeTwoPointers(command1);
        freeTwoPointers(command2);
        exit(EXIT_FAILURE);
      }
      childPid1 = fork();
      if (childPid1 < 0) {
        perror("Error in forking process for child 1.");
        // free memories
        freeTwoPointers(command1);
        freeTwoPointers(command2);
        exit(EXIT_FAILURE);
      }
      if (childPid1 == 0) {
        char **args1 = rePipeWrite(command1);
        freeTwoPointers(command1);
        if (args1 == NULL) {
          freeTwoPointers(command2);
          exit(EXIT_FAILURE);
        }
        close(fd[0]);
        int dup2Ret1 = dup2(fd[1], STDOUT_FILENO);
        if (dup2Ret1 < 0) {
          perror("Error in dup2 proccess in child1.");
          freeTwoPointers(command2);
          freeTwoPointers(args1);
          exit(EXIT_FAILURE);
        }
        close(fd[1]);
        if (execvp(args1[0], args1) == -1) {
          freeTwoPointers(command2);
          freeTwoPointers(args1);
          perror("Error in executing child process1");
          exit(EXIT_FAILURE);
        }
        freeTwoPointers(args1);
      }
      childPid2 = fork();
      if (childPid2 < 0) {
        perror("Error in creating child process 2.");
        freeTwoPointers(command2);
        exit(EXIT_FAILURE);
      }
      if (childPid2 == 0) {
        char **args2 = rePipeRead(command2);
        freeTwoPointers(command2);
        if (args2 == NULL) {
          exit(EXIT_FAILURE);
        }
        close(fd[1]);
        int dup2Ret2 = dup2(fd[0], STDIN_FILENO);
        if (dup2Ret2 < 0) {
          perror("Error in dup2 proccess in child1.");
          freeTwoPointers(args2);
          exit(EXIT_FAILURE);
        }
        close(fd[0]);
        if (execvp(args2[0], args2) == -1) {
          freeTwoPointers(args2);
          perror("Error in executing child process2");
          exit(EXIT_FAILURE);
        }
        freeTwoPointers(args2);
      }
      if ((childPid2 != 0) && (childPid1 != 0)) {
       close(fd[1]);
        do {
          if (waitpid(childPid1,&status1,0) == -1) {
            freeTwoPointers(command);
            freeTwoPointers(command1);
            freeTwoPointers(command2);
            perror("Error in child process 1 termination");
            exit(EXIT_FAILURE);
          }
          if (waitpid(childPid2, &status2,0) == -1) {
            freeTwoPointers(command);
            freeTwoPointers(command1);
            freeTwoPointers(command2);
            perror("Error in child process 2 termination");
            exit(EXIT_FAILURE);
          }
        } while ((!WIFEXITED(status1) &&
                 !WIFSIGNALED(status1)) || (!WIFEXITED(status2) &&
                 !WIFSIGNALED(status2)));
        childPid1 = 0;
        childPid2 = 0;
      }
      freeTwoPointers(command1);
      freeTwoPointers(command2);
    }
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
 * Checks if EOF (Ctrl + D) is being read and exits penn-shredder if that is the
 * case Otherwise, it checks for a valid input and adds the characters to an
 * input buffer.
 *
 * From this input buffer, the first 1023 characters (if more than 1023) or the
 * whole buffer are assigned to command and returned. An \0 is appended to the
 * command so that it is null terminated after getting the command, it will
 * break the command into tokens, and store it into
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
        if(input[i] != ' '){
            break;
        }
        leadingSpace++;
    }
    if((input[readIn-1] == '\n') && (readIn - 1 == leadingSpace)){
        free(input);
        return NULL;
    }
    int trailingSpace = 0;
    for(int i = readIn - 1;i >= 0; i--){
        if((input[i] != ' ') && (input[i] != '\n')){
            break;
        }
        trailingSpace++;
    }
    command = malloc(sizeof(char) * (readIn + 1));
    int j = 0;
    for(int i = leadingSpace; i < (readIn - trailingSpace); i++){
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
