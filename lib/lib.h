#include <stdio.h>
#include "../interface/interface.h"
#include "../helpers/helpers.h"
#include <unistd.h>
#include <time.h>
#include <unistd.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <stdlib.h>
#ifndef _SHELL_LIBS_
#define _SHELL_LIBS_

bool is_built_in(cmd_t cmd);
void process_built_in(cmd_t cmd);
void process_cmd_simple(cmd_t cmd);
void process_cmd_fileout(cmd_t cmd);
void process_cmd_pipe(cmd_t cmd);
void handle_SIGINT(int sig);
void handle_SIGHUP();
void signal_handler(int sig);
void process_cmd_background(cmd_t cmd);
void exec_shell();

#endif