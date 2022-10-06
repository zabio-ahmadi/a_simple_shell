#include <stdio.h>
#include <stdlib.h>
#include <errno.h>

// equivalent to a function that would do perror and exit
// The do /while is a trick so that the macro can be executed in all kind of context
//(e.g. in a if without brackets)
#define die_errno(msg)   \
    do                   \
    {                    \
        int err = errno; \
        perror(msg);     \
        exit(err);       \
    } while (0)
