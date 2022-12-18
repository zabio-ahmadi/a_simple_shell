#include "lib.h"
#include <errno.h>

#define SIZE 3

pid_t child_pid[SIZE]; // global array to store process IDs of child processes
int proc_index = 0;    // global variable to keep track of number of child processes

// function to copy contents of a file identified by file descriptor fd_from to a new file specified by 'to' argument
int copy(const int fd_from, const char *to)
{
  int fd_to;
  char buf[4096];
  ssize_t nread;

  // open 'to' file for writing with exclusive access and permissions 0x777
  fd_to = open(to, O_WRONLY | O_EXCL, 0x777);

  // if 'to' file could not be opened
  if (fd_to < 0)
  {
    // save error number and close 'from' file
    int savedError = errno;
    close(fd_from);
    // print error message to stderr and return -1
    fprintf(stderr, "Could not open the file %s: %s\n", to, strerror(savedError));
    return -1;
  }

  // read data from 'from' file in 4096-byte blocks and write to 'to' file
  while (nread = read(fd_from, buf, sizeof buf), nread > 0)
  {
    char *out_ptr = buf;
    ssize_t nwritten;
    do
    {
      // write data from 'buf' to 'to' file
      nwritten = write(fd_to, out_ptr, nread);
      // if write was successful
      if (nwritten >= 0)
      {
        // reduce number of bytes left to write by number of bytes written
        nread -= nwritten;
        // move pointer to next block of data to be written
        out_ptr += nwritten;
      }
      // if write was interrupted by a signal
      else if (errno != EINTR)
      {
        // save error number, close 'from' and 'to' files, and print error message
        int savedError = errno;
        close(fd_from);
        close(fd_to);
        fprintf(stderr, "Could not copy to %s: %s\n", to, strerror(savedError));
        return -1;
      }
    } while (nread > 0); // repeat until all data has been written
  }

  // if there was an error reading from 'from' file
  if (nread != 0)
  {
    // save error number, close 'from' and 'to' files, and print error message
    int savedError = errno;
    close(fd_from);
    close(fd_to);
    fprintf(stderr, "Could not read: %s\n", strerror(savedError));
    return -1;
  }

  // close 'from' and 'to' files
  close(fd_from);
  close(fd_to);
  return 0; // return success
}

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
    copy(STDOUT_FILENO, "/dev/null");
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
    // Set a signal handler for SIGCHLD
    signal(SIGCHLD, signal_handler);
  }
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
    signal(sig, SIG_IGN);
    break;
  // If sig is SIGQUIT
  case SIGQUIT:
    // Set the signal handler for SIGQUIT to ignore the signal
    signal(sig, SIG_IGN);
    break;

  // If sig is SIGCHLD
  case SIGCHLD:
    // Set the signal handler for SIGCHLD to ignore the signal
    signal(sig, SIG_IGN);
    break;
  // If sig is SIGINT
  case SIGINT:
    // Send sig to all child processes in child_pid using kill()
    for (int i = 0; i < proc_index; i++)
      if (child_pid[i] != -1)
      {
        kill(child_pid[i], sig);
        child_pid[i] = -1;
      }
    // Wait for any child process to exit
    waitpid(-1, NULL, 1);
    break;
  // If sig is SIGHUP
  case SIGHUP:
    // // Send SIGTERM to all child processes in child_pid using kill()
    for (int i = 0; i < proc_index; i++)
      if (child_pid[i] != -1)
      {
        kill(child_pid[i], sig);
        child_pid[i] = -1;
      }
    // // Declare a variable to store the exit status of the child process
    // int child_status;
    // // Wait for any child process to exit
    // int err = waitpid(-1, &child_status, 1);
    // printf("there is an error : %d\n", err);
    // // If the child process terminated normally, print its exit status and exit the program
    // if (WIFEXITED(child_status))
    // {
    //   printf("Foreground job exited with code %d\n", WEXITSTATUS(child_status));
    // }
    // exit(0);

    while (waitpid(-1, NULL, WNOHANG) > 0)
      ;
    exit(0);
    break;
  default:
    break;
  }
}

void exec_shell()
{

  // struct sigaction sa;
  // sa.sa_handler = signal_handler;
  // sigemptyset(&sa.sa_mask);
  // sigaddset(&sa.sa_mask, SIGHUP);
  // sa.sa_flags = SA_SIGINFO | SA_RESTART;
  // sigaction(SIGTERM, &sa, NULL);
  // sigaction(SIGQUIT, &sa, NULL);
  // sigaction(SIGINT, &sa, NULL);
  // sigaction(SIGHUP, &sa, NULL);

  for (size_t i = 0; i < SIZE; i++)
  {
    child_pid[i] = -1;
  }

  signal(SIGTERM, signal_handler);
  signal(SIGQUIT, signal_handler);
  signal(SIGINT, signal_handler);
  signal(SIGHUP, signal_handler);

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
      if (cmd.type == CMD_SIMPLE && cmd.foreground == true)
        process_cmd_simple(cmd);
      // If cmd is a command with output redirection to a file, process it
      if (cmd.type == CMD_FILEOUT)
        process_cmd_fileout(cmd);
      // If cmd is a command with a pipe, process it
      if (cmd.type == CMD_PIPE)
        process_cmd_pipe(cmd);
      // If cmd is a command to be run in the background, process it
      if (cmd.foreground == false)
      {
        process_cmd_background(cmd);
      }
      // Free the resources used by cmd
      dispose_command(&cmd);
    }
  }
}
