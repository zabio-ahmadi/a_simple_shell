#include "helpers.h"

void printInColor(char *color, char *string)
{
    printf("%s%s", color, string);
}
void control_key_handler(int sig)
{
    signal(sig, SIG_IGN);
    int err = system("clear");

    if (err == -1)
        fprintf(stderr, "command error\n");

    printInColor(cyan, "voulez vous vraiment quitter ? [oui/non] : ");
    char c;
    c = getchar();
    if (c == 'o' || c == 'O')
        exit(0);
}
