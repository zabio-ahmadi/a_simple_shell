#include "lib/lib.h"
#include <stdlib.h>

void sighup()

{
    signal(SIGHUP, sighup); /* reset signal */
    printf("CHILD: I have received a SIGHUP\n");
}

void sigint()

{
    signal(SIGINT, sigint); /* reset signal */
    printf("CHILD: I have received a SIGINT\n");
}

void sigquit()

{
    printf("My DADDY has Killed me!!!\n");
    exit(0);
}
int main()
{

    // signal(SIGTERM, sighup); /* set function calls */
    // signal(SIGINT, sigint);
    // signal(SIGQUIT, sigquit);
    exec_shell();
    exit(EXIT_SUCCESS);
}
