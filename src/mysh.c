// Implement a basic UNIX shell in C: recursive version

// The shell should parse tokens from the command line with strtok() and execute the command with execv().
// The shell should support the following built-in commands:
// - cd
// - exit
// - pwd

// The shell should support the following redirection operators:
// - < (redirect stdin)
// - > (redirect stdout)
// - A | B (pipe stdout of command A to stdin of command B)

// The shell should support the following special characters:
// - * (wildcard)

// After tokenizing the input, the shell should check if the first token is a built-in command.
// If it is, the shell should execute the built-in command.
// Else, if the first token is a path to an executable, the shell should execute the executable.
    // This should be done with stat(), not by traversing the directory tree.
// Else, if the first token is a path to an executable with a wildcard, the shell should execute the executable.
    // This should be done with stat(), not by traversing the directory tree.
// Else, the shell should check if the first token is a valid command in the following directories, in order:
// - /usr/local/sbin
// - /usr/local/bin
// - /usr/sbin
// - /usr/bin
// - /sbin
// - /bin
// If it is, the shell should execute the executable.
// Else, the shell should print an error message and set the last error status to 1.
// Also, if at any point the shell encounters an error, it should print an error message and set the last error status to 1.

// prompt_user(char* prompt);
// read_line(char* line, char* stdin_string, char* stdout_string);
// check_command();
// execute_command();

// // Run a loop of:
// // - Print a prompt
// // - Read a line of input
// // - Parse the input into tokens
// // - Check if the input is a valid command
// // - Execute the command
// // - Repeat
// int main() {

// }

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>

#define MAX_ARGS 20
#define MAX_LENGTH 100

int execute_command(char* args[], int input_fd, int output_fd);

int main() {
  char line[MAX_LENGTH];
  char* args[MAX_ARGS];
  int input_fd = STDIN_FILENO;
  int output_fd = STDOUT_FILENO;
  int i;

  while (1) {
    // Print prompt and read input
    printf("mysh> ");
    if (fgets(line, MAX_LENGTH, stdin) == NULL) {
      break;
    }

    // Parse input into arguments
    char* token = strtok(line, " \n");
    i = 0;
    while (token != NULL && i < MAX_ARGS - 1) {
      args[i] = token;
      token = strtok(NULL, " \n");
      i++;
    }
    args[i] = NULL;

    // Check for built-in commands
    if (strcmp(args[0], "cd") == 0) {
      if (args[1] == NULL) {
        chdir(getenv("HOME"));
      } else if (chdir(args[1]) == -1) {
        perror("cd");
      }
      continue;
    } else if (strcmp(args[0], "exit") == 0) {
      exit(EXIT_SUCCESS);
    } else if (strcmp(args[0], "pwd") == 0) {
        char* cwd = getcwd(NULL, 0);
        if (cwd == NULL) {
            perror("pwd");
        } else {
            printf("%s\n", cwd);
            free(cwd);
        }
        continue;
    }

    // Check for redirection operators
    for (i = 0; args[i] != NULL; i++) {
        if (strcmp(args[i], "<") == 0) {
            args[i] = NULL;
            i++;
            if (args[i] == NULL) {
                fprintf(stderr, "Error: missing input file!\n");
                goto end_loop;
            }
            input_fd = open(args[i], O_RDONLY);
            if (input_fd == -1) {
                perror("open");
                goto end_loop;
            }
        } else if (strcmp(args[i], ">") == 0) {
                args[i] = NULL;
                i++;
            if (args[i] == NULL) {
                fprintf(stderr, "Error: missing output file!\n");
                goto end_loop;
            }
            output_fd = open(args[i], O_WRONLY | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR);
            if (output_fd == -1) {
                perror("open");
                goto end_loop;
            }
        }
    }

    // Execute command
    if (execute_command(args, input_fd, output_fd) == -1) {
      goto end_loop;
    }

    // Cleanup redirection
    if (input_fd != STDIN_FILENO) {
      close(input_fd);
      input_fd = STDIN_FILENO;
    }
    if (output_fd != STDOUT_FILENO) {
      close(output_fd);
      output_fd = STDOUT_FILENO;
    }

    end_loop:
        // Reset arguments
        for (i = 0; args[i] != NULL; i++) {
        args[i] = NULL;
        }
  }

  return EXIT_SUCCESS;
}

// Returns 0 on success, -1 on error
int execute_command(char* args[], int input_fd, int output_fd) {
    pid_t pid = fork();
    if (pid == -1) {
        perror("fork");
        return -1;
    } else if (pid == 0) {
        // Child process
        if (input_fd != STDIN_FILENO) {
            dup2(input_fd, STDIN_FILENO);
            close(input_fd);
        }
   
        if (output_fd != STDOUT_FILENO) {
            dup2(output_fd, STDOUT_FILENO);
            close(output_fd);
        }

        // Try to execute command with stat()
        struct stat st;
        if (stat(args[0], &st) == 0 && S_ISREG(st.st_mode) && (st.st_mode & S_IXUSR)) {
            execv(args[0], args);
            perror("execv");
            return -1;
        }

        // Try to execute command with wildcard and stat()
        int i;
        char* path = NULL;
        for (i = 0; args[0][i] != '\0'; i++) {
            if (args[0][i] == '*') {
                path = malloc((i + 6) * sizeof(char));
                if (path == NULL) {
                    perror("malloc");
                    return -1;
                }
                strncpy(path, args[0], i);
                strcpy(path + i, "./*");
                break;
            }
        }
        if (path != NULL && stat(path, &st) == 0 && S_ISREG(st.st_mode) && (st.st_mode & S_IXUSR)) {
            args[0] = path;
            execv(args[0], args);
            perror("execv");
            free(path);
            return -1;
        }
        free(path);

        // Try to execute command in directories
        char* directories[] = {"/usr/local/sbin", "/usr/local/bin", "/usr/sbin", "/usr/bin", "/sbin", "/bin"};
        char filename[MAX_LENGTH];
        for (i = 0; i < sizeof(directories) / sizeof(char*); i++) {
            snprintf(filename, MAX_LENGTH, "%s/%s", directories[i], args[0]);
            if (stat(filename, &st) == 0 && S_ISREG(st.st_mode) && (st.st_mode & S_IXUSR)) {
                args[0] = filename;
                execv(args[0], args);
                perror("execv");
                return -1;
            }
        }

        // Command not found
        fprintf(stderr, "Error: command not found: %s\n", args[0]);
        return -1;
    } else {
        // Parent process
        int status;
        // Use wait
        if (wait(&status) == -1) {
            perror(args[0]);
            return -1;
        }
        return 0;
    }
}