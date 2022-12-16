#include "lib.h"
#include <errno.h>

// pid_t child_pid;

#define SIZE 10
pid_t child_pid[SIZE];
int proc_index = 0;

int copy(const int fd_from, const char *to)
{
  int fd_to;
  char buf[4096];
  ssize_t nread;

  fd_to = open(to, O_WRONLY | O_EXCL, 0x777);

  if (fd_to < 0)
  {
    int savedError = errno;
    close(fd_from);
    fprintf(stderr, "Could not open the file %s: %s\n",
            to, strerror(savedError));
    return -1;
  }

  while (nread = read(fd_from, buf, sizeof buf), nread > 0)
  {
    char *out_ptr = buf;
    ssize_t nwritten;
    do
    {
      nwritten = write(fd_to, out_ptr, nread);
      if (nwritten >= 0)
      {
        nread -= nwritten;
        out_ptr += nwritten;
      }
      else if (errno != EINTR)
      {
        int savedError = errno;
        close(fd_from);
        close(fd_to);
        fprintf(stderr, "Could not copy to %s: %s\n", to, strerror(savedError));
        return -1;
      }
    } while (nread > 0);
  }

  if (nread != 0)
  {
    int savedError = errno;
    close(fd_from);
    close(fd_to);
    fprintf(stderr, "Could not read: %s\n", strerror(savedError));
    return -1;
  }

  close(fd_from);
  close(fd_to);
  return 0;
}

bool is_built_in(cmd_t cmd)
{
  return ((strcmp(cmd.argv[0], "exit") == 0) || (strcmp(cmd.argv[0], "cd") == 0));
}
void process_built_in(cmd_t cmd)
{
  /**************************************************************************************
   * BUILT-IN COMMAND                                                                   *
   * EXIT                                                                               *
   **************************************************************************************/
  if (strcmp(cmd.argv[0], "exit") == 0)
    exit(EXIT_SUCCESS);

  /**************************************************************************************
   * BUILT-IN : change directory                                                                          *
   **************************************************************************************/
  if (strcmp(cmd.argv[0], "cd") == 0)
    if (chdir(cmd.argv[1]) != 0)
      fprintf(stderr, "erreur : can't change directory");
}

void process_cmd_simple(cmd_t cmd)
{
  pid_t pid = fork();
  child_pid[proc_index++] = pid;
  // is child
  if (pid == 0)
  {
    // ignore other code and execute only the paramter passed by argv by program stored in argv[0]
    // inspired by documentation code example
    if (execvp(cmd.argv[0], cmd.argv) < 0) // (first index value of the string array, array)
    {
      fprintf(stderr, "erreur d'execution de la commande\n");
      // perror("erreur d'execution de la commande")
      exit(EXIT_FAILURE);
    }
  }
  // parent
  else
  {
    int child_status;
    waitpid(pid, &child_status, 0);
    if (WIFEXITED(child_status))
    {
      printf("Foreground job exited with code %d\n", WEXITSTATUS(child_status));
    }
  }
}

void process_cmd_fileout(cmd_t cmd)
{
  pid_t pid = fork();
  child_pid[proc_index++] = pid;
  if (pid == 0)
  {
    int destination_fd;
    // OWRONLY : write only
    // O_TRUNC : delete file -> recreate file if exists
    destination_fd = open(cmd.argv2[0], O_WRONLY | O_CREAT | O_TRUNC, 0774);
    // read permisison in base octal

    if (destination_fd < 0)
    {
      fprintf(stderr, "CANNOT OPEN FILE DESCRIPTOR OF %s \n", cmd.argv2[0]);
    }

    if (dup2(destination_fd, STDOUT_FILENO) < 0)
    {
      fprintf(stderr, "CANNOT OPEN COPY %s \n", cmd.argv2[0]);
    }

    if (execvp(cmd.argv[0], cmd.argv) < 0) // (first index value of the string array, array)
    {
      fprintf(stderr, "erreur d'execution de la commande\n");
      // perror("erreur d'execution de la commande")
      exit(EXIT_FAILURE);
    }
    close(destination_fd);
  }
  else
  {
    int child_status;
    waitpid(pid, &child_status, 0);
    if (WIFEXITED(child_status))
    {
      printf("Foreground job exited with code %d\n", WEXITSTATUS(child_status));
    }
  }
}

void process_cmd_pipe(cmd_t cmd)
{
  pid_t pid1, pid2;
  int fd[2];
  if (pipe(fd) < 0)
    perror("cannot pipe\n");

  pid1 = fork();
  child_pid[proc_index++] = pid1;
  if (pid1 < 0)
    perror("failed to create process 1\n");

  if (pid1 == 0)
  {
    close(fd[1]);
    dup2(fd[0], STDIN_FILENO);
    close(fd[0]);
    if (execvp(cmd.argv2[0], cmd.argv2) < 0)
      perror("command failed\n");
  }
  else
  {
    pid2 = fork();
    child_pid[proc_index++] = pid2;
    if (pid2 < 0)
      perror("failed to create process 2\n");

    if (pid2 == 0)
    {
      close(fd[0]);
      dup2(fd[1], STDOUT_FILENO);
      close(fd[1]);
      if (execvp(cmd.argv[0], cmd.argv) < 0)
        perror("command failed\n");
    }

    close(fd[0]);
    close(fd[1]);

    int child_1_status, child_2_status;
    waitpid(pid1, &child_1_status, 0);
    waitpid(pid2, &child_2_status, 0);

    if (WIFEXITED(child_1_status))
    {
      printf("Foreground job exited with code %d\n", WEXITSTATUS(child_1_status));
    }
    if (WIFEXITED(child_2_status))
    {
      printf("Foreground job exited with code %d\n", WEXITSTATUS(child_2_status));
    }
  }
}

void process_cmd_background(cmd_t cmd)
{
  pid_t pid = fork();
  // is child
  child_pid[proc_index++] = pid;
  if (pid == 0)
  {

    // rediriger l’entrée standard du job vers /dev/null pour éviter les conflits avec le shell
    copy(STDOUT_FILENO, "/dev/null");

    // ignore other code and execute only the paramter passed by argv by program stored in argv[0]
    // inspired by documentation code example
    if (execvp(cmd.argv[0], cmd.argv) < 0) // (first index value of the string array, array)
    {
      fprintf(stderr, "erreur d'execution de la commande\n");
      // perror("erreur d'execution de la commande")
      exit(EXIT_FAILURE);
    }
  }
  else if (pid < 0)
  {
    fprintf(stderr, "canot fork\n");
  }
  // parent
  else
  {
    signal(SIGCHLD, signal_handler);
  }
}

// handler

void signal_handler(int sig)
{
  switch (sig)
  {
  case SIGTERM:
    signal(SIGTERM, SIG_IGN);
    break;
  case SIGQUIT:
    signal(SIGQUIT, SIG_IGN);
    break;

  case SIGCHLD:
    signal(SIGCHLD, SIG_IGN);
    break;
  case SIGINT:
    for (int i = 0; i < proc_index; i++)
      if (child_pid[i] != 0)
        kill(child_pid[i], sig);
    break;
  case SIGHUP:
    // kill all child processus correclty
    for (int i = 0; i < proc_index; i++)
      if (child_pid[i] != 0)
        kill(child_pid[i], SIGTERM);

    int child_status;
    // wait for any child process to exit
    waitpid(-1, &child_status, 0);

    if (WIFEXITED(child_status))
      printf("Foreground job exited with code %d\n", WEXITSTATUS(child_status));
    exit(0);
    break;
  default:
    break;
  }
}

void exec_shell()
{

  // ignore signals
  signal(SIGTERM, signal_handler);
  signal(SIGQUIT, signal_handler);
  signal(SIGINT, signal_handler);
  signal(SIGHUP, signal_handler);

  cmd_t cmd;
  char user_input[MAX_INPUT_SIZE] = {};
  while (1)
  {
    // optimize performance
    // sched_yield();

    if (ask_user_input(user_input) > 0)
    {

      parse_command(user_input, &cmd);
      bool isbuiltin = is_built_in(cmd);

      if (isbuiltin)
        process_built_in(cmd);

      if (cmd.type == CMD_SIMPLE && cmd.foreground == true)
        process_cmd_simple(cmd);

      if (cmd.type == CMD_FILEOUT)
        process_cmd_fileout(cmd);

      if (cmd.type == CMD_PIPE)
        process_cmd_pipe(cmd);
      if (cmd.foreground == false)
      {
        process_cmd_background(cmd);
      }

      dispose_command(&cmd); // free
    }
  }
}
