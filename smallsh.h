#ifndef SHELL_H
#define SHELL_H

#include <stdio.h>      
#include <stdlib.h>     
#include <string.h>     
#include <unistd.h>     
#include <sys/types.h>  
#include <sys/wait.h>   
#include <fcntl.h>      
#include <signal.h>     
#include <errno.h>      

#define MAX_CHAR_LENGTH 2048
#define MAX_ARGUEMENTS 512


// Command struct to hold individual command components
typedef struct Command {
    // This is the array of arguments
    char* args[MAX_ARGUEMENTS];
    // This is the number of arguements
    int numArgs;

    // This is for the redirection using input and output
    char* inputFile;
    char* outputFile;

    // This for for the background process on if it is true or false.
    // I tried using a boolean but it didn't work so I used an integer instead for this
    // as it works using 1s and 0s for defining true and false.
    int background;
} 

Command;

// We are allowing background processes to run
extern int bgAllowed;

// These are the functions that I have set up in the code
// I will actually go through each one of these in the functions themselves
// As of right now these are declarations
char* readInput();
void handleTokens(char* token, Command* command);
void runProgram(Command* command, int* prevStatus);
int commandHandler(Command* command, int* prevStatus);
void exitCommand(Command* command);
void cdCommand(Command* command);
void statusCommand(int prevStatus);
void SIGCHLDhandler(int sigNum);
void SIGINThandler(int sigNum);
void SIGTSTPhandler(int sigNum);
void childProcess(Command* command);
void parentProcess(pid_t child_pid, Command* command, int* prevStatus);

#endif
