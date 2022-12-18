#include "lib.h"
#include <errno.h>

#define SIZE 10

pid_t child_pid[SIZE]; // global array to store process IDs of child processes
int proc_index = 0;    // global variable to keep track of number of child processes

// function definition for "is_built_in" taking in a "cmd_t" object called "cmd"
bool is_built_in(cmd_t cmd)
{
  // return a boolean value indicating whether the first argument of the "cmd" object is either "exit" or "cd"
  return ((strcmp(cmd.argv[0], "exit") == 0) || (strcmp(cmd.argv[0], "cd") == 0));
}
// function definition for "process_built_in" taking in a "cmd_t" object called "cmd"
void process_built_in(cmd_t cmd)
{
  if (strcmp(cmd.argv[0], "exit") == 0) // if the first argument of the "cmd" object is "exit"
    exit(EXIT_SUCCESS);                 // exit the program with a success status

  if (strcmp(cmd.argv[0], "cd") == 0)                     // if the first argument of the "cmd" object is "cd"
    if (chdir(cmd.argv[1]) != 0)                          // if changing the current working directory to the second argument of the "cmd" object fails
      fprintf(stderr, "erreur : can't change directory"); // print an error message to "stderr"
}

// function definition for "process_cmd_simple" taking in a "cmd_t" object called "cmd"
void process_cmd_simple(cmd_t cmd)
{
  pid_t pid = fork();            // create a new process (fork) and store the process id in a variable called "pid"
  child_pid[proc_index++] = pid; // store the process id in the "child_pid" array at the current "proc_index" and increment "proc_index"

  // if the current process is a child process
  if (pid == 0)
  {
    // execute the program stored in the first element of the "cmd.argv" array with the remaining elements as arguments
    // if execution fails, print an error message to "stderr" and exit the program with a failure status
    if (execvp(cmd.argv[0], cmd.argv) < 0)
    {
      fprintf(stderr, "erreur d'execution de la commande\n");
      exit(EXIT_FAILURE);
    }
  }
  // if the current process is the parent process
  else
  {
    int child_status;               // variable to store the exit status of the child process
    waitpid(pid, &child_status, 0); // wait for the child process with process id "pid" to finish and store its exit status in "child_status"
    if (WIFEXITED(child_status))    // if the child process terminated normally
    {
      printf("Foreground job exited with code %d\n", WEXITSTATUS(child_status)); // print the exit code of the child process
    }
  }
}

// function definition for "process_cmd_fileout" taking in a "cmd_t" object called "cmd"
void process_cmd_fileout(cmd_t cmd)
{
  pid_t pid = fork();            // create a new process (fork) and store the process id in a variable called "pid"
  child_pid[proc_index++] = pid; // store the process id in the "child_pid" array at the current "proc_index" and increment "proc_index"

  if (pid == 0) // if the current process is a child process
  {
    int destination_fd; // variable to store the file descriptor for the destination file

    // open the destination file for writing, creating it if it doesn't exist and truncating its contents if it does exist
    destination_fd = open(cmd.argv2[0], O_WRONLY | O_CREAT | O_TRUNC, 0774);

    // if the file couldn't be opened
    if (destination_fd < 0)
    {
      fprintf(stderr, "CANNOT OPEN FILE DESCRIPTOR OF %s \n", cmd.argv2[0]); // print an error message to "stderr"
    }

    // redirect stdout to the destination file
    if (dup2(destination_fd, STDOUT_FILENO) < 0)
    {
      fprintf(stderr, "CANNOT OPEN COPY %s \n", cmd.argv2[0]); // print an error message to "stderr"
    }

    // execute the program stored in the first element of the "cmd.argv" array with the remaining elements as arguments
    // if execution fails, print an error message to "stderr" and exit the program with a failure status
    if (execvp(cmd.argv[0], cmd.argv) < 0)
    {
      fprintf(stderr, "erreur d'execution de la commande\n");
      exit(EXIT_FAILURE);
    }

    close(destination_fd); // close the file descriptor for the destination file
  }
  else // if the current process is the parent process
  {
    int child_status;               // variable to store the exit status of the child process
    waitpid(pid, &child_status, 0); // wait for the child process with process id "pid" to finish and store its exit status in "child_status"
    if (WIFEXITED(child_status))    // if the child process terminated normally
    {
      printf("Foreground job exited with code %d\n", WEXITSTATUS(child_status)); // print the exit code of the child process
    }
  }
}

void process_cmd_pipe(cmd_t cmd)
{
  // Declare two variables to store the process IDs of the child processes
  // and an array to store the file descriptor for the pipe
  pid_t pid1, pid2;
  int fd[2];

  // Create a pipe and check for errors
  if (pipe(fd) < 0)
    perror("cannot pipe\n");

  // Create a child process
  pid1 = fork();

  // Store the process ID of the child process in an array
  child_pid[proc_index++] = pid1;

  // Check for errors in creating the child process
  if (pid1 < 0)
    perror("failed to create process 1\n");

  // If the current process is the child process created by fork()
  if (pid1 == 0)
  {
    // Close the write end of the pipe
    close(fd[1]);

    // Redirect the read end of the pipe to stdin
    dup2(fd[0], STDIN_FILENO);

    // Close the read end of the pipe
    close(fd[0]);

    // Execute the second command in cmd using execvp()
    if (execvp(cmd.argv2[0], cmd.argv2) < 0)
      perror("command failed\n");
  }
  // If the current process is the parent process
  else
  {
    // Create another child process
    pid2 = fork();
    // Store the process ID of the second child process in the array
    child_pid[proc_index++] = pid2;
    // Check for errors in creating the second child process
    if (pid2 < 0)
      perror("failed to create process 2\n");

    // If the current process is the second child process created by fork()
    if (pid2 == 0)
    {
      // Close the read end of the pipe
      close(fd[0]);
      // Redirect the write end of the pipe to stdout
      dup2(fd[1], STDOUT_FILENO);
      // Close the write end of the pipe
      close(fd[1]);
      // Execute the first command in cmd using execvp()
      if (execvp(cmd.argv[0], cmd.argv) < 0)
        perror("command failed\n");
    }

    // If the current process is the parent process of both child processes
    // Close both ends of the pipe
    close(fd[0]);
    close(fd[1]);

    // Declare variables to store the exit status of the child processes
    int child_1_status, child_2_status;

    // Wait for the first child process to terminate and store its exit status
    waitpid(pid1, &child_1_status, 0);

    // Wait for the second child process to terminate and store its exit status
    waitpid(pid2, &child_2_status, 0);

    // If the first child process terminated normally, print its exit status
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
  // Create a child process
  pid_t pid = fork();
  // If the current process is the child process
  if (pid == 0)
  {
    // Redirect stdout to /dev/null to avoid conflicts with the shell
    int dev_null = open("/dev/null", O_WRONLY, 0777);
    dup2(dev_null, STDOUT_FILENO);
    close(dev_null);

    // Execute the command specified in cmd using execvp()
    if (execvp(cmd.argv[0], cmd.argv) < 0)
    {
      // Print an error message if the execution fails
      fprintf(stderr, "erreur d'execution de la commande\n");
      // Exit with a failure status
      exit(EXIT_FAILURE);
    }
  }
  // If the call to fork() fails
  else if (pid < 0)
  {
    // Print an error message
    fprintf(stderr, "canot fork\n");
  }
  // If the current process is the parent process
  else
  {
    signal(SIGCHLD, signal_handler);
  }
}

void handle_SIGCHILD()
{
  int status;
  waitpid(-1, &status, WNOHANG);

  if (WIFEXITED(status))
    printf("Background job exited with code %d\n", WEXITSTATUS(status));
}

void handle_SIGINT(int sig)
{
  for (int i = 0; i < proc_index; i++)
  {
    if (child_pid[i] != -1)
      kill(child_pid[i], sig);
  }

  waitpid(-1, NULL, WNOHANG);
  proc_index = 0;
}

void handle_SIGHUP()
{
  // kill all child processus correclty
  for (int i = proc_index - 1; i >= 0; i--)
    if (child_pid[i] != -1)
      kill(child_pid[i], SIGTERM);

  int child_status;
  // wait for any child process to exit
  // don't block waiting
  waitpid(-1, &child_status, WNOHANG);

  if (WIFEXITED(child_status))
    printf("Foreground job exited with code %d\n", WEXITSTATUS(child_status));
  proc_index = 0;
  exit(0);
}

// handler
void signal_handler(int sig)
{
  // Check the value of sig
  switch (sig)
  {
  // If sig is SIGTERM
  case SIGTERM:
    // Set the signal handler for SIGTERM to ignore the signal
    signal(SIGTERM, SIG_IGN);
    break;

  // If sig is SIGQUIT
  case SIGQUIT:
    // Set the signal handler for SIGQUIT to ignore the signal
    signal(SIGQUIT, SIG_IGN);
    break;

  // If sig is SIGCHLD
  case SIGCHLD:
    // call handler for SIGCHLD
    handle_SIGCHILD();
    break;

  // If sig is SIGINT
  case SIGINT:
    // call handler for SIGCHLD
    handle_SIGINT(sig);
    break;
  case SIGHUP:
    handle_SIGHUP();
    break;
  }
}

void exec_shell()
{

  // configure child process id table
  for (size_t i = 0; i < SIZE; i++)
  {
    child_pid[i] = -1;
  }
  proc_index = 0;

  // Set up the signal handler
  struct sigaction sa;
  sa.sa_handler = signal_handler;
  sigemptyset(&sa.sa_mask);
  sa.sa_flags = SA_RESTART;
  if (sigaction(SIGTERM, &sa, NULL) == -1)
  {
    perror("sigaction : SIGTERM");
    exit(EXIT_FAILURE);
  }

  if (sigaction(SIGQUIT, &sa, NULL) == -1)
  {
    perror("sigaction : SIGQUIT");
    exit(EXIT_FAILURE);
  }

  if (sigaction(SIGINT, &sa, NULL) == -1)
  {
    perror("sigaction : SIGINT");
    exit(EXIT_FAILURE);
  }

  if (sigaction(SIGHUP, &sa, NULL) == -1)
  {
    perror("sigaction : SIGHUP");
    exit(EXIT_FAILURE);
  }

  // Declare a variable to store the parsed command
  cmd_t cmd;
  // Declare an array to store user input
  char user_input[MAX_INPUT_SIZE] = {};
  // Enter an infinite loop
  while (1)
  {
    // Get user input and store it in user_input
    if (ask_user_input(user_input) > 0)
    {
      // Parse the user input and store the result in cmd
      parse_command(user_input, &cmd);
      // Check if the command is a built-in command
      bool isbuiltin = is_built_in(cmd);
      // If the command is a built-in command, process it
      if (isbuiltin)
        process_built_in(cmd);
      // If cmd is a simple command to be run in the foreground, process it
      else if (cmd.type == CMD_SIMPLE && cmd.foreground == true)
        process_cmd_simple(cmd);
      // If cmd is a command with output redirection to a file, process it
      else if (cmd.type == CMD_FILEOUT)
        process_cmd_fileout(cmd);
      // If cmd is a command with a pipe, process it
      else if (cmd.type == CMD_PIPE)
        process_cmd_pipe(cmd);
      // If cmd is a command to be run in the background, process it
      else if (cmd.foreground == false)
      {
        process_cmd_background(cmd);
      }
      // Free the resources used by cmd
      dispose_command(&cmd);
    }
  }
}
