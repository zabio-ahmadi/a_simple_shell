#include "lib.h"

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

void exec_shell()
{
  cmd_t cmd;
  char user_input[MAX_INPUT_SIZE] = {};
  while (1)
  {
    if (ask_user_input(user_input) > 0)
    {
      parse_command(user_input, &cmd);
      bool isbuiltin = is_built_in(cmd);

      if (isbuiltin)
        process_built_in(cmd);

      if (cmd.type == CMD_SIMPLE)
        process_cmd_simple(cmd);

      if (cmd.type == CMD_FILEOUT)
        process_cmd_fileout(cmd);

      if (cmd.type == CMD_PIPE)
        process_cmd_pipe(cmd);

      dispose_command(&cmd); // free
    }
  }
}