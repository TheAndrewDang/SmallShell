// Name: Andrew Dang

#include "smallsh.h"

int bgAllowed = 1;

// The main function ( I did see the professor use void main() before but it doesn't work here)
int main(){
    // The status of the last foreground process
    int prevStatus = 0;

    // I stole this off lecture slides 3.3 by the way shh

    // This is the initializers for SIGINT, SIGTSTP, and SIGCHLD
    // We are setting these to 0 at the start
    struct sigaction SIGINT_action = {0}, SIGTSTP_action = {0}, SIGCHLD_action = {0};

    // This is for ignoring the Ctrl-C functionality
    // What this should do is send the action to SIG_IGN
    // To which it should ignored
    SIGINT_action.sa_handler = SIG_IGN;

    // This is set up to catch SIGINT signal Ctrl+C
    // The reason why this is used is because when Ctrl+C
    // is used it should also kill the any children in the
    // foreground but still ignore the children in the background
    SIGINT_action.sa_handler = SIGINThandler;

    // This sets the signal handler for SIGTSTP to handle SIGTSTP signal
    // Ctrl+Z is for entering and exiting for the foreground modes
    SIGTSTP_action.sa_handler = SIGTSTPhandler;

    // This is to handle SIGCHLD signal, sent when child processes stop or terminate
    SIGCHLD_action.sa_handler = SIGCHLDhandler;

    // This is the blocker to all other signals during the execution of the SIGCHLD signal handler
    sigfillset(&SIGCHLD_action.sa_mask);

    // This sets the SA_RESTART flag for the SIGCHLD_action handler
    // Which will automatically restart system calls
    // https://www.gnu.org/software/libc/manual/html_node/Flags-for-Sigaction.html
    SIGCHLD_action.sa_flags = SA_RESTART;

    // Puts the associated signals with their proper handlers
    sigaction(SIGINT, &SIGINT_action, NULL);
    sigaction(SIGTSTP, &SIGTSTP_action, NULL);
    sigaction(SIGCHLD, &SIGCHLD_action, NULL);

    // This keeps the loop running in main
    while(1){
        // Command structure to hold the current command
        // While also initializing this command
        Command command = {0};

        // Read the user input
        char* input;
        // Set input to be the results of readInput function
        input = readInput();

        // If the input line is empty or starts with '#'
        // Just continue going forward and skip it
        if(input[0] == '\0' || input[0] == '#'){
            continue;
        }

        // Tokenize the input line
        char* token = strtok(input, " ");

        // Handle the tokens
        handleTokens(token, &command);

        // If the command is not a built-in command ctoninue to run the program
        if(!commandHandler(&command, &prevStatus)){
            runProgram(&command, &prevStatus);
        }
    }
    return 0;
}

// Reads the input line from the user
char* readInput(){
    // Allocates memory for the input buffer
    // The reason why I did this is because it sets the right amount of
    // memory needed for the amount of characters inputed specifically the max character length of 2048
    // I could throw in 2048 in there but I already declared the variable earlier so might as well use it
    char* buffer = malloc(sizeof(char) * MAX_CHAR_LENGTH);

    // This is the input line letting you know you're using the shell
    // As well as letting you know you can input stuff
    printf(": ");

    // The ketchup and mustard, the printf to fflush
    fflush(stdout);

    // Reading the input line
    fgets(buffer, MAX_CHAR_LENGTH, stdin);

    // Look for new line character and if not found it returns buffer
    // Mainly used to replace the newline with a null
    buffer[strcspn(buffer, "\n")] = 0;

    return buffer;
}

// Handles the tokens from the input line
void handleTokens(char* token, Command* command){
    // Setting up a temporary string with the length of 2048
    char tempString[MAX_CHAR_LENGTH];

    // This is created as a counter for a for loop
    int i;

    // Starts a loop going through till the token isn't null
    while(token != NULL){
        // fill the tempString with a bunch of 0s to ensure no residual data is
        // in tempString before we use it
        memset(tempString, 0, sizeof(tempString));

        // For loop starts looking through the token
        for(i = 1; i < strlen(token); i++){
            // If current and previous characters are both $
            if(token[i] == '$' && token[i-1] == '$'){
                // Copy the part of the string before $$
                strncpy(tempString, token, i - 1);
                // Append the process id
                sprintf(tempString + strlen(tempString), "%d", getpid());
                // Append the rest of the original string after $$
                strcpy(tempString + strlen(tempString), token + i + 1);
                // Replaces the original token with the new string
                token = tempString;
                // gtfo
                break;
            }
        }

        // If the token has the < in it that is when we will activate input redirection
        if (strcmp(token, "<") == 0){
            // Get the next token after the <
            token = strtok(NULL, " ");

            // Make sure its not empty
            if (token != NULL){
                // If not empty change the command inputFile value to the token
                command->inputFile = token;
            }
        }

        // We do the exact same thing here with input redirection but this time it is
        // with output redirection instead so no need for me to repeat myself
        else if (strcmp(token, ">") == 0){
            token = strtok(NULL, " ");
            if (token != NULL){
                command->outputFile = token;
            }
        } 

        // This is to indicate background execution
        else if (strcmp(token, "&") == 0){

            // If we allow it then set the command structs background to 1
            if(bgAllowed){
                command->background = 1;
            }
        } 


        else {
            // If the command didn't have the special characters from above
            // Then we just move the token into the command args then
            // we make sure to add one more to the number of arguements
            command->args[command->numArgs++] = token;
        }

        // Then get the next token
        token = strtok(NULL, " ");
    }

    // Sets the next position in the args array (after all the arguments) to NULL
    command->args[command->numArgs] = NULL;
}

// Handles running a program simple stuff
void runProgram(Command* command, int* prevStatus){
    // Fork a new process
    pid_t childPid = fork();

    // If the child PID is 0
    if(childPid == 0){
        // Execute the childProcess function
        childProcess(command);
    } 
    else{
        // Or else we will executre the parentProcess
        parentProcess(childPid, command, prevStatus);
    }
}

// Handles execution in the child process
void childProcess(Command* command){
    // Make sure that the are running in the foreground
    if (!command->background || !bgAllowed){
        // This initializes and sets the Ctrl+C back to its default
        struct sigaction child_SIGINT_action = {0};
        // https://stackoverflow.com/questions/3645172/how-to-reset-sigint-to-default-after-pointing-it-some-user-defined-handler-for-s
        // http://odl.sysworks.biz/disk$axpdocdec971/progtool/deccv56/5763profile0024.html
        // SIG_DFL can do that for us
        child_SIGINT_action.sa_handler = SIG_DFL;
        // Calling sigaction will have it associate the signal with this action
        sigaction(SIGINT, &child_SIGINT_action, NULL);
    }

    // With this process we got to make it be in the background
    // as this will redirect standard input to /dev/null
    if(command->background && bgAllowed){
        // This opens the file in a read only mode (O_RDONLY)
        // puts the value into devNull
        int devNull = open("/dev/null", O_RDONLY);
        // Call dup2 to duplicate the file descriptor
        // If it returns back -1
        if(dup2(devNull, STDIN_FILENO) == -1){
            // These errors will occur if dup2 fails
            perror("dup2");
            exit(1);
        }
        // close devNull
        close(devNull);
    }

    // Almost the same as the previous but this one does
    // Input redirection
    if(command->inputFile != NULL){
        // Opens the current inputFile in read only
        int fileDescriptor = open(command->inputFile, O_RDONLY);
        // In the case that its not true then this executes
        if(fileDescriptor == -1){
            printf("cannot open %s for input\n", command->inputFile);
            fflush(stdout);
            exit(1);
        }
        // Did the same with the above as this is a dup2 check
        if(dup2(fileDescriptor, STDIN_FILENO) == -1){
            perror("dup2");
            exit(1);
        }
        // Close it
        close(fileDescriptor);
    }

    // Same thing but with a little bit more
    // This is output redirection instead
    if(command->outputFile != NULL){
        // Setting the file descriptor to open the outputFile
        // We give these the abilities of write only, creating if non existent, truncating the file to 0 if
        // it already exists, and the 0644 is for the permission mode
        // https://www.namecheap.com/support/knowledgebase/article.aspx/400/205/file-permissions/
        int fileDescriptor = open(command->outputFile, O_WRONLY | O_CREAT | O_TRUNC, 0644);
        // Same thing as last time where we have these if statements to catch failing results
        if(fileDescriptor == -1){
            printf("cannot open %s for output\n", command->outputFile);
            fflush(stdout);
            exit(1);
        }
        // same thing as before
        if(dup2(fileDescriptor, STDOUT_FILENO) == -1){
            perror("dup2");
            exit(1);
        }
        // Close it of course
        close(fileDescriptor);
    }
    // Slides 3.1
    // https://www.qnx.com/developers/docs/6.5.0SP1.update/com.qnx.doc.neutrino_lib_ref/e/execvp.html
    // This checks if the execvp can run
    if(execvp(command->args[0], command->args) == -1){
        // This will output if it fails
        printf("bash: %s: no such file or directory\n", command->args[0]);
        fflush(stdout);
        exit(1);
    }
}

void parentProcess(pid_t childPid, Command* command, int* prevStatus){
    // Foreground only and background is not allowed
    if(!command->background || !bgAllowed){
        // More stuff I stole off the 3.1 & 3.2 slides
        // This just waits for the child process to stop
        waitpid(childPid, prevStatus, 0);

        // This checks to see if the line was terminated by a signal
        if(WIFSIGNALED(*prevStatus)){
            printf("foreground pid %d is done: ", childPid);
            statusCommand(*prevStatus);
        }
    } 
    else{
        // Print background process id
        printf("background pid is %d\n", childPid);
        fflush(stdout);
    }
}

int commandHandler(Command* command, int* prevStatus){
    // This is just a nice place for the 3 commands we were supposed to impliment
    if(strcmp(command->args[0], "exit") == 0){
        // If this is exit then we do the exit function
        exitCommand(command);
        return 1;
    } 
    else if(strcmp(command->args[0], "cd") == 0){ 
        // If the input is cd then we execute the cd function
        cdCommand(command);
        return 1;
    } 
    else if(strcmp(command->args[0], "status") == 0){
        // If the input is status then we execute the status function
        statusCommand(*prevStatus);
        return 1;
    }
    // If none of these are used then just return 0
    return 0;
}

// Handles the exit command
// This thing just exits pretty simple
void exitCommand(Command* command){
    exit(0);
}

void cdCommand(Command* command){
    // If there is no argument then go to the home directory
    if(command->args[1] == NULL){
        chdir(getenv("HOME"));
    } 
    else{
        // If chdir fails then we print an error
        if(chdir(command->args[1]) != 0){
            perror("chdir");
        }
    }
}

void statusCommand(int prevStatus){
    // If the command ended normally then we will print the exit status
    if(WIFEXITED(prevStatus)){
        printf("exit value %d\n", WEXITSTATUS(prevStatus));
    }
    // If the command was terminated by a signal then we will print the signal number
    else if(WIFSIGNALED(prevStatus)){
        printf("terminated by signal %d\n", WTERMSIG(prevStatus));
    }
    // Never forget
    fflush(stdout);
}

void SIGCHLDhandler(int sigNum){
    // The int just stores the exit status
    int childExitMethod;
    // This stores the PID of the child processs
    pid_t childPid;
        // Checks if the processes have completed until it has returned 0
        while((childPid = waitpid(-1, &childExitMethod, WNOHANG)) > 0){
            // Print the pid and the exit status of the terminated background process
            printf("background pid %d is done: ", childPid);
            // Get the status number
            statusCommand(childExitMethod);
        }
}

// This one simply just handles the Ctrl+C stuff whenever it is used
void SIGINThandler(int sigNum){
    char* message = "\nTerminated by signal 2\n";
    write(STDOUT_FILENO, message, 22);
}

// I think of this as the on and off switch for foreground only mode
void SIGTSTPhandler(int sigNum){
    // Toggle the state of the bgAllowed
    if(bgAllowed){
        char* message = "\nEntering foreground-only mode (& is now ignored)\n";
        write(STDOUT_FILENO, message, 50);
        // Switch to make sure the background is no longer allowed
        bgAllowed = 0;
    } 
    else{
        char* message = "\nExiting foreground-only mode\n";
        write(STDOUT_FILENO, message, 30);
        // Make sure the background is allowed
        bgAllowed = 1;
    }
}
