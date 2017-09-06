#include <signal.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include "parser.h"

#define BUILT_IN_CD 1
#define BUILT_IN_PWD 2
#define BUILT_IN_EXPORT 3 
#define BUILT_IN_ECHO 4 
#define BUILT_IN_WITHOUT_EXPORT 5
#define TRUE 1
#define FALSE 0
//#define DEBUG TRUE
#define BUFFER_SIZE 1000
#define MAX_PS1_LENGTH 100


char PS1[MAX_PS1_LENGTH] = "sbush~>";

int execvpe(const char *file, char *const argv[], char *const envp[]);
pid_t waitpid(pid_t pid, int *stat_loc, int options);
char *secure_getenv(const char *name);
void custom_fputs(char * chr, FILE * out);

static int child = 0;

static void gracefulExit(const char* msg) {
  (child ? _exit : exit)(EXIT_FAILURE);
}

void waitForProcessExecution(int pid) { 
      int waitStatus =0;    
      do {
        waitpid(pid, &waitStatus, WUNTRACED);
      } while (!WIFEXITED(waitStatus) && !WIFSIGNALED(waitStatus));
}


void setExecutionArguments(char *envp[], char *argv[], commandArgument *c_arg) {
  char *path = getenv("PATH");
  envp[0] = path;
  envp[1] = NULL;
  argv[0] = (*c_arg).command;
  int index = 0;
  for (; index < (*c_arg).argumentCount; index++) {
   argv[index + 1] = (*c_arg).arguments[index];
  }
  argv[index + 1] = NULL;
}

void executeBinaryInBackGround(commandArgument *c_arg) {
  char *envp[2];
  char *argv[(*c_arg).argumentCount + 2];
  setExecutionArguments(envp, argv, c_arg);
  int pid = fork();
  if (pid == 0) {
    setpgid(0,0);
    execvpe((*c_arg).command, argv, envp);
    custom_fputs(strerror(errno), stdout);
  } else  {
     struct sigaction sigchld_action = {
       .sa_handler = SIG_DFL,
       .sa_flags = SA_NOCLDWAIT
     };
    sigaction(SIGCHLD, &sigchld_action, NULL);
  }
}

void executeBinaryInteractively(commandArgument *c_arg) {
  char *envp[2];
  char *argv[(*c_arg).argumentCount + 2];
  setExecutionArguments(envp, argv, c_arg);
  int pid = fork();
  if (pid == 0) {
    int status = execvpe((*c_arg).command, argv, envp);
    if (status != 0) {
      custom_fputs((*c_arg).command, stdout);
      custom_fputs(": command not found.\n", stdout);
    }
    exit(0);
  } else {
      waitForProcessExecution(pid);
  }
}


void executeBinary(commandArgument *c_arg) {
  if ((*c_arg).isBackground) {
    executeBinaryInBackGround(c_arg);
  } else {
    executeBinaryInteractively(c_arg);
  }
}


void executeBuiltInPwd(commandArgument *c_arg) {
 if ((*c_arg).argumentCount > 1) {
  custom_fputs("No argument is allowed for pwd.", stdout);
  custom_fputs("\n", stdout);
  return;
 }
  char cwd[1024];
  if (getcwd(cwd, sizeof(cwd)) != NULL) {
   custom_fputs(cwd, stdout);
   custom_fputs("\n", stdout);
  }
}

void executeBuiltInEcho(commandArgument *c_arg)
{
  for(int i = 0; i< (*c_arg).argumentCount; i++)
  {
    char *c_dollar = strchr((*c_arg).arguments[i], '$');
    if(c_dollar)
    {
      char * result = secure_getenv(c_dollar + 1);
      custom_fputs(result, stdout);
      custom_fputs("\n",stdout);
    }
  }
}

void executeBuiltInExport(commandArgument *c_arg)
{
  for(int i = 0; i< (*c_arg).argumentCount; i++)
  {
    char *c_equal = strchr((*c_arg).arguments[i], '=');
    if(c_equal)
    {
      *c_equal = '\0';
      setenv((*c_arg).arguments[i], c_equal + 1, 1);
    }
  }
}


void executeBuiltInWithoutExport(commandArgument *c_arg)
{
  char *c_equal = strchr((*c_arg).command, '=');
  if(c_equal)
  {
    (*c_arg).command[c_equal - (*c_arg).command] = '\0';
    char *value = (*c_arg).command + (c_equal - (*c_arg).command) + 1;
    setenv((*c_arg).command, value, 1);
    if(strcmp((*c_arg).command, "PS1") == 0)
    {
      memset(PS1,'\0', 100);
      memcpy(PS1, value, strlen(value));
    }
  }
  executeBuiltInExport(c_arg);
}

void executeBuiltInCd(commandArgument *c_arg ) {
 if ((*c_arg).argumentCount > 1) {
  custom_fputs("Max 1 argument is allowed for cd.", stdout);
  custom_fputs("\n", stdout);
  return;
 } else if ((*c_arg).argumentCount == 1) {
   int result = chdir((*c_arg).arguments[0]);
   if (result != 0) {
     custom_fputs(strerror(errno), stdout);
     custom_fputs("\n", stdout);
    }
  }  
  else {
     int result = chdir("/");
     if (result != 0) {
       custom_fputs(strerror(errno), stdout);
       custom_fputs("\n", stdout);
     }
 }
}

int getBuiltInCode(commandArgument * c_arg) {
 if (strcmp("cd", (*c_arg).command) == 0) {
  return (int) BUILT_IN_CD;
 } else if (strcmp("pwd", (*c_arg).command) == 0) {
  return (int) BUILT_IN_PWD;
 } else if (strcmp("export", (*c_arg).command) == 0) {
  return (int) BUILT_IN_EXPORT;
 } else if (strcmp("echo", (*c_arg).command) == 0) {
  return (int) BUILT_IN_ECHO;
 } else if (strchr((*c_arg).command, '=') != NULL) {
  return (int) BUILT_IN_WITHOUT_EXPORT;
 }

 return 0;
}


void executeCommand(commandArgument * c_arg) {
  int builtInCode = getBuiltInCode(c_arg);
  if(builtInCode) {
    switch (builtInCode) {
      case 1 :
              executeBuiltInCd(c_arg);
              break;
      case 2 :
              executeBuiltInPwd(c_arg);
              break;
      case 3 :
             executeBuiltInExport(c_arg);
             break;
      case 4 :
             executeBuiltInEcho(c_arg);
             break;
      case 5 :
             executeBuiltInWithoutExport(c_arg);
             break;
      default:
           custom_fputs("BuiltIn is not implemented", stdout);
           custom_fputs("\n", stdout);
    }
  } else {
     executeBinary(c_arg);
  }
}


void printParsedCommand(commandArgument * c_Arg) {
  if (c_Arg != NULL) {
    custom_fputs("command : ", stdout);
    custom_fputs((*c_Arg).command, stdout);
    custom_fputs("\n", stdout);
    for (int i = 0; i < (*c_Arg).argumentCount; i++) {
       custom_fputs("Argument ", stdout);
       custom_fputs(" : ", stdout);
       custom_fputs((*c_Arg).arguments[i], stdout);
       custom_fputs("\n", stdout);
    }
  } else {
   custom_fputs("parsed command is NULL", stdout);
   custom_fputs("\n", stdout);
  }

}

void custom_fputs(char * chr, FILE * out) {
 if (chr != NULL) {
   fputs(chr, out);
 }
}


void duplicateFileDescriptorsFromInputToOutput(int old_fd, int new_fd) {
  if (old_fd == new_fd) {
     // do nothing
  } else {
     if (dup2(old_fd, new_fd) != -1) {
       close(old_fd);
     } else {
      custom_fputs("error while executing pipes", stdout);
    }  
  }
}

void runPiping(commandArgument *c_Args[], int numberOfCommands, int currentCommandIndex, int file_desc) {
  if ((currentCommandIndex + 1) <= numberOfCommands) {
    if ((currentCommandIndex + 1) == numberOfCommands) {
      duplicateFileDescriptorsFromInputToOutput(file_desc, 0);
      executeBinary(c_Args[currentCommandIndex]);
      gracefulExit("execvp last");
    } else {
      int fd[2];
      if ( pipe(fd) == -1) {
        custom_fputs("Error while creating pipe", stdout);
      }
      switch(fork()) {
        case -1:
           custom_fputs("Some error occured while forking", stdout);
           custom_fputs("\n", stdout);
           gracefulExit("fork");
        case 0:
           close(fd[0]);
           duplicateFileDescriptorsFromInputToOutput(file_desc, 0);
	   duplicateFileDescriptorsFromInputToOutput(fd[1], 1);
           executeBinary(c_Args[currentCommandIndex]);
           gracefulExit("execvp");
        default:
          // close(file_desc);
           close(fd[1]);
           runPiping(c_Args, numberOfCommands, currentCommandIndex + 1, fd[0]); 
      }
    }
  }
}



int countNumberOfPipes(char * input) {
 int count = 0;
 while (*input) {
  if(*input == '|') {
   count++;
  }
  input++;
 }
 return count;
}

void freeAllParsedCommandArguments(commandArgument *c_Args[], int numOfPipes) {
 for (int index = 0; index <= numOfPipes; index++) {
   if ((c_Args[index]) != NULL) {
     freeCommandArgument(c_Args[index]);
   }
 }
}

int isValidateParsedPipeInput(commandArgument *c_Args[], int numOfPipes) {
 for (int index = 0; index <= numOfPipes; index++) {
   if (strlen((*c_Args[index]).command) == 0) {
     return 0;
   }
 } 
 return 1;
}

void parsePipeCommand(commandArgument *c_Args[], int numOfPipes, char * input) {
   char * prev = input;
   char * next;
   for (int i = 0 ; i <= numOfPipes; i++) {
     next = strchr(prev, '|');
     if (next != NULL) { 
       *next = '\0';
     } 
     c_Args[i] = parseInput(prev, ' ');
     if (next != NULL) {
       prev = next + 1;
     } else {
       next = strchr(prev, '\0');
       prev = next + 1;
     }
   }
}


int main(int argc, char *argv[], char *envp[]) {
 
  char buffer[BUFFER_SIZE];
  while (TRUE) {
   custom_fputs(PS1, stdout);

   char * input = fgets(buffer, BUFFER_SIZE, stdin);

   if (input == NULL || *input == '\n') {
     continue;
   }

   if (!strcmp(input, "exit\n")) {
     custom_fputs("Exiting as requested", stdout);
     custom_fputs("\n", stdout);
     return 1;
    }

   if(strchr(input, '|') != NULL) {
     // Implement piping support
      int numOfPipes = countNumberOfPipes(input);
      commandArgument *c_Args[numOfPipes + 1];
      parsePipeCommand(c_Args, numOfPipes, input);
      if (isValidateParsedPipeInput(c_Args, numOfPipes)) {
        if (signal(SIGCHLD, SIG_IGN) == SIG_ERR) {
               gracefulExit("signal");
             }
        int pgid = tcgetpgrp(0);
        int c_pid = fork();
        switch(c_pid){
         case 0:
           runPiping(c_Args, numOfPipes + 1, 0, 0);
           exit(0);
         default:
             //waitpid(c_pid, &status, WNOHANG);
             waitForProcessExecution(c_pid);
             tcsetpgrp(0, pgid); 
             continue;
         }
      } else {
         custom_fputs("Invalid pipe syntex. Try Again", stdout);
         custom_fputs("\n", stdout);
      }
      freeAllParsedCommandArguments(c_Args, numOfPipes);
   } else {
    commandArgument *c_Arg = parseInput(input, ' ');
    #ifdef DEBUG
      printParsedCommand(c_Arg);
    #endif
    executeCommand(c_Arg);
    freeCommandArgument(c_Arg);
    }
  }
  return 0;
}

