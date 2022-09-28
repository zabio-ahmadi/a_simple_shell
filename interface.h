#ifndef _INTERFACE_H
#define _INTERFACE_H
#include <stdbool.h>


#define ARG_MAX 1024 //maximum line length in the terminal

typedef enum cmdtype {CMD_SIMPLE, CMD_PIPE, CMD_FILEOUT, CMD_FILEIN, CMD_FILEAPPEND} cmdtype_t;

// Defines a full user command
typedef struct cmd {
  char** argv;        // a first command splitted as arguments in the argv style
  int argc;           // number of arguments in the first command
  char** argv2;       // a second command in argv style, can be either the second command of a pipe or the filename of a file redirection
  int argc2;          // the number of arguments in the second command
  cmdtype_t type;     // type of the command (see enum, pipe. file redirection, etc.)
  bool foreground;    // should the command be run in foreground or background (might not be relevant for your TP depending on TP version)
} cmd_t;

//Parse a user command into its arguments
//IN:
//  user_input: a string containing the full command. IT WILL BE MODIFIED BY THE FUNCTION.
//OUT:
//  cmd: command updated from user input string (i.e. command input). MUST BE DISPOSED SEE FUNCTION dispose_command
//  return: -1 in case of error otherwise 0.
int parse_command(char *user_input, cmd_t *cmd);

//Wait for the user input and return it
//OUT:
//  user_input: a string with the user input. The user should take care of the
//              allocation.
//  return: 0 if an empty input was given, -1 if the function failed to read stdin
int ask_user_input(char* user_input);

//Dispose of the command, which free some allocated memory
//A COMMAND MUST BE DISPOSED BEFORE IT IS USED FOR A NEW COMMAND (i.e. before calling parse again)
//IN:
//  cmd: the command to dispose of
void dispose_command(cmd_t *cmd);

//Print command for debugging
void print_command(cmd_t cmd);

#endif //_INTERFACE_H