#include <stdio.h>
#include <string.h>
#include <signal.h>
#include <stdlib.h>
#ifndef _SHELL_HELPER__
#define _SHELL_HELPER__
#define MAX_INPUT_SIZE 400
void control_key_handler(int sig);
#define red "\033[1;31m"
#define green "\033[0;32m"
#define blue "\033[0;34m"
#define yellow "\033[1;33m"
#define purple "\033[0;35m"
#define cyan "\033[0;36m"
#define black "\033[0;30m"
#define white "\033[0;37m"

void printInColor(char *color, char *string);

#endif
