#include "helpers/helpers.h"
#include "interface/interface.h"
#include "interface/common.h"
#include "lib/lib.h"

#include <unistd.h>
#include <time.h>
#include <unistd.h>
#include <sys/wait.h>

// static sig_atomic_t flag;

int main()
{
    /**************************************************************************************
     * CAPTUER SIGINT SIGNAL                                                              *
     **************************************************************************************/
    signal(SIGINT, control_key_handler);

    cmd_t cmd;

    char user_input[MAX_INPUT_SIZE] = {};
    while (1)
    {
        if (ask_user_input(user_input) > 0)
        {
            /**************************************************************************************
             * PARSE USER COMMAND AND PUT IT INSIDE OUR STRUCTURE                                 *
             **************************************************************************************/
            parse_command(user_input, &cmd);

            /**************************************************************************************
             * BUILT-IN COMMAND                                                                   *
             **************************************************************************************/
            if (strcmp(cmd.argv[0], "exit") == 0)
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
                /**************************************************************************************************
                 * FORK : FIRST METHOD TO CREATE A CHILD PROCESSUS
                 * CHILD PROCESSUS: IS ALMOST A COPY OF PARENT PROCESSUS (RESTRICTED SHARED CONTENT -> READ-ONLY)
                 * PID: 0
                 * PPID OF CHILD == PID OF PARENT
                 **************************************************************************************************/
                pid_t pid = fork();

                if (pid == 0)
                {
                    /**************************************************************************************
                     * CHILDPROCESSUS: child processus block                                              *
                     * EXEC* : EXECUT THE CODE OF A CHILD PROCESSUS                                       *
                     * EXECVP: RETURN -1 IF ERROR AND ERRNO IS SET TO INDICATE THE ERROR                  *
                     * EXIT: SEND SIGCHLD SIGNAL & FREE THE FILE DESCRIPTOR                               *
                     **************************************************************************************/

                    if (execvp(cmd.argv[0], cmd.argv) < 0) // (first index value of the string array, array
                    {
                        fprintf(stderr, "erreur d'execution de la commande");
                        exit(EXIT_FAILURE);
                    }
                }

                else
                {
                    /**************************************************************************************
                     * PARENTPROCESSUS: parent processus block                                            *
                     **************************************************************************************/

                    /********************************************************************************************
                     * WAITPID: IT BLOCK PARENT TO WAIT TO CHILD PROCESS TO FINISH                              *
                     * The wait() system call suspends execution of the calling process                         *
                     * until one of its children terminates                                                     *
                     * wait until child send SIGCHLD & close file descripteur file                              *
                     *<-1: wait for any child process whose process GID is equal to the absolute value of pid   *
                     * -1: meaning wait for any child process.                                                  *
                     *  0: wait for any child process whose process GID ID equals to that of the calling process*
                     * >0: meaning wait for the child whose process ID is equal to the value of pid.            *
                     ********************************************************************************************/

                    /********************************************************************************************************
                     * WIFEXITED: return exit code of child *                                                               *
                     * Evaluates to a non-zero value if status was returned for a child process that terminated normally.   *
                     * 0: for normale exit                                                                                  *
                     ********************************************************************************************************/

                    int child_status;
                    waitpid(pid, &child_status, 0);
                    if (WIFEXITED(child_status)) //
                    {
                        printf("Foreground job exited with code %d\n", WEXITSTATUS(child_status));
                    }
                }
            }
            dispose_command(&cmd); // free
        }
    }
    exit(EXIT_SUCCESS);
}
