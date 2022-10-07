#include "helpers/helpers.h"
#include "interface/interface.h"
#include "interface/common.h"
#include "lib/lib.h"

#include <unistd.h>
#include <time.h>
#include <unistd.h>
#include <sys/wait.h>

static sig_atomic_t flag;

int main()
{
    flag = 0;
    // capture SIGKINT
    signal(SIGINT, control_key_handler);
    cmd_t cmd;
    char user_input[500] = {};
    while (1)
    {

        // int p_err = ask_user_input(user_input);

        // if (p_err == -1 || p_err == 0)
        //     continue;

        if (ask_user_input(user_input) > 0)
        {
            parse_command(user_input, &cmd);

            // command builtin
            if (strcmp(*cmd.argv, "exit") == 0)
            {
                exit(EXIT_SUCCESS);
            }

            if (strcmp(cmd.argv[0], "cd") == 0)
            {

                int err = chdir(cmd.argv[1]);

                if (err != 0)
                {
                    fprintf(stderr, "erreur : can't change directory ");
                }
            }

            else
            {
                pid_t pid = fork();

                if (pid == 0)
                {
                    // printf("id: %d, child\n", pid);
                    if (execvp(cmd.argv[0], cmd.argv) < 0) // (first index value of the string array, array
                    {
                        fprintf(stderr, "erreur d'execution de commande");
                        exit(EXIT_FAILURE);
                    }
                }

                else
                {
                    int child_status;
                    // printf("id: %d, parent\n", pid);
                    waitpid(pid, &child_status, 0);
                    if (WIFEXITED(child_status)) // wait until child send SIGCHLD
                    {
                        printf("Foreground job exited with code %d\n", WEXITSTATUS(child_status));
                    }
                }
            }
        }
        dispose_command(&cmd); // free
    }
    exit(EXIT_SUCCESS);
}
