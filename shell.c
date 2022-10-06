#include "helpers/helpers.h"
#include "interface/interface.h"
#include "interface/common.h"
#include "lib/lib.h"

#include <limits.h>
#include <unistd.h>
#include <time.h>
#include <unistd.h>
#include <sys/wait.h>

static sig_atomic_t flag;

int main()
{
    cmd_t cmd;
    flag = 0;
    // capture SIGKINT
    signal(SIGINT, control_key_handler);
    while (1)
    {

        char user_input[ARG_MAX] = {};
        if (ask_user_input(user_input) != 0)
        {
            parse_command(user_input, &cmd);

            pid_t pid = fork();

            if (pid == 0)
            {
                printf("id: %d, child\n", pid);
                // system(*cmd.argv);
                execvp(*cmd.argv, cmd.argv); // (first index value of the string array, array)

                perror("program exit with error");
                exit(EXIT_SUCCESS);
            }
            if (pid > 0)
            {
                printf("id: %d, parent\n", pid);
                int status;
                waitpid(pid, &status, 0);

                if (WIFEXITED(status)) // wait until child send SIGCHLD
                {

                    printf("Foreground job exited with code %d\n", status);
                }
            }
        }
    }
    dispose_command(&cmd); // free
    exit(EXIT_SUCCESS);
}

// double time_spent = 0.0;
// clock_t begin = clock();

//     clock_t end = clock();
//     time_spent += (double)(end - begin) / CLOCKS_PER_SEC;

//     printf("..............................................................................................................took %fs\n", time_spent);
