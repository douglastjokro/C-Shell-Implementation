#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void reSTDOUT( char *tokenAfter);

void reSTDIN(char* tokenAfter);

void freeTwoPointers(char **twoPointers);

int isPipe(char** command);

char** createTokenArrayBefore (char** command);

char** createTokenArrayAfter (char** command);

char** rePipeWrite(char** command1);

char** rePipeRead(char** command2);
