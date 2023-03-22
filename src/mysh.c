#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <dirent.h>
#include <errno.h>
#include <signal.h>
#include <stdbool.h>


// Global variables for the shell program
char *prompt; // The prompt to be displayed to the user
char *curr_dir; // The current working directory of the shell program
int exit_status; // The exit status of the last command executed by the shell program

// Function prototypes for the shell program
void init_shell(); // Initializes the shell program by setting up the prompt and current working directory, and setting the exit status to 0
void read_command(char *command); // Reads a command from standard input and stores it in command, which is assumed to be large enough to hold it (i.e., at least 256 characters)
void parse_command(char *command, char **argv); // Parses command into an array of arguments, argv, which is assumed to be large enough to hold them (i.e., at least 256 elements)
void execute_command(char **argv); // Executes the command specified by argv, which is assumed to be a valid command name or path name followed by its arguments (if any)

// Function prototypes for built-in commands
void cd(char **argv); // Changes the current working directory of the shell program to argv[1] if it exists and is accessible, or prints an error message otherwise and sets exit_status to 1 if argv[1] is not specified or does not exist or is inaccessible; otherwise, sets exit_status to 0
void pwd(); // Prints the current working directory of the shell program to standard output and sets exit_status to 0


// Function prototypes for other functions used by execute_command()
bool is_builtin(char *command); // Returns 1 if command is a built-in command, 0 otherwise; does not modify exit_status
bool is_executable(char *command); // Returns 1 if command is an executable file in one of the directories in PATH, 0 otherwise; does not modify exit_status


// Function prototypes for other functions used by execute_command() and is_executable()
int check_dir(char *dirname, char *filename); // Returns 1 if filename exists in dirname and can be executed, 0 otherwise; does not modify exit_status

// Function prototypes for other functions used by execute_command() and parse_command()
int contains(char *str1, char *str2); // Returns 1 if str1 contains str2 as a substring, 0 otherwise; does not modify exit_status

// Main function for the shell program
int main() {
    // Initialize the shell program
    init_shell();

    // Run the shell program
    while (1) {
        // Print the prompt
        printf("\n%s> ", prompt);

        // Read a command from standard input
        char command[256];
        read_command(command);

        // Parse the command into an array of arguments
        char *argv[256];
        parse_command(command, argv);

        // Execute the command
        execute_command(argv);
    }

    // Exit the shell program
    return 0;
}

// Reads a command from standard input and stores it in command, which is assumed to be large enough to hold it (i.e., at least 256 characters)
void read_command(char *command) {
    // Read a command from standard input
    fgets(command, 256, stdin);

    // Remove the newline character at the end of the command
    command[strlen(command) - 1] = '\0';

}

// Initializes the shell program by setting up the prompt and current working directory, and setting the exit status to 0
void init_shell() {
    // Set the prompt to "mysh" by default
    prompt = "mysh";

    // Set the current working directory to the current working directory of the shell program
    curr_dir = getcwd(NULL, 0);

    // Set the exit status to 0
    exit_status = 0;

    // Print a welcome message
    printf("Welcome to mysh!");

    // Print the prompt
    printf("%s", prompt);

    // Read in input from the user
    char command[256];
    read_command(command);

    // Parse the command into an array of arguments
    char *argv[256];
    parse_command(command, argv);

    // Execute the command
    execute_command(argv);

    // Print the prompt
    printf("%s", prompt);
}

 // Parses command into an array of arguments, argv, which is assumed to be large enough to hold them (i.e., at least 256 elements)
void parse_command(char *command, char **argv) {
    // Initialize the array of arguments to NULL
    for (int i = 0; i < 256; i++) {
        argv[i] = NULL;
    }

    // Parse the command into an array of arguments
    int i = 0;
    char *token = strtok(command, " ");
    while (token != NULL) {
        argv[i] = token;
        token = strtok(NULL, " ");
        i++;
    }

    // If the command contains a pipe, parse the command into an array of arguments for the first command
    if (contains(command, "|")) {
        // Initialize the array of arguments to NULL
        for (int i = 0; i < 256; i++) {
            argv[i] = NULL;
        }

        // Parse the command into an array of arguments for the first command
        int i = 0;
        char *token = strtok(command, "|");
        while (token != NULL) {
            argv[i] = token;
            token = strtok(NULL, "|");
            i++;
        }
    }

    // If the command contains a redirection operator, parse the command into an array of arguments for the first command
    if (contains(command, ">") || contains(command, "<")) {
        // Initialize the array of arguments to NULL
        for (int i = 0; i < 256; i++) {
            argv[i] = NULL;
        }

        // Parse the command into an array of arguments for the first command
        int i = 0;
        char *token = strtok(command, ">");
        while (token != NULL) {
            argv[i] = token;
            token = strtok(NULL, ">");
            i++;
        }
    }

    // If the command contains a redirection operator, parse the command into an array of arguments for the first command
    if (contains(command, ">") || contains(command, "<")) {
        // Initialize the array of arguments to NULL
        for (int i = 0; i < 256; i++) {
            argv[i] = NULL;
        }

        // Parse the command into an array of arguments for the first command
        int i = 0;
        char *token = strtok(command, "<");
        while (token != NULL) {
            argv[i] = token;
            token = strtok(NULL, "<");
            i++;
        }
    }
}

// Executes the command specified by argv, which is assumed to be a valid command name or path name followed by its arguments (if any)
void execute_command(char** argv) {
    // If the command is a built-in command, execute it
    if (is_builtin(argv[0])) {
        // If the command is "cd", execute it
        if (strcmp(argv[0], "cd") == 0) {
            cd(argv);
        }

        // If the command is "pwd", execute it
        if (strcmp(argv[0], "pwd") == 0) {
            pwd();
        }
    }

    // If the command is not a built-in command, execute it
    else {
        // If the command is an executable file in one of the directories in PATH, execute it
        if (is_executable(argv[0])) {
            // Fork a child process
            pid_t pid = fork();

            // If the child process was successfully created, execute the command
            if (pid == 0) {
                // If the command contains a pipe, execute the command
                if (contains(argv[0], "|")) {
                    // Parse the command into an array of arguments for the first command
                    char *argv1[256];
                    parse_command(argv[0], argv1);

                    // Parse the command into an array of arguments for the second command
                    char *argv2[256];
                    parse_command(argv[1], argv2);

                    // Create a pipe
                    int pipefd[2];
                    pipe(pipefd);

                    // Fork a child process
                    pid_t pid = fork();

                    // If the child process was successfully created, execute the first command
                    if (pid == 0) {
                        // Close the read end of the pipe
                        close(pipefd[0]);

                        // Duplicate the write end of the pipe to standard output
                        dup2(pipefd[1], STDOUT_FILENO);

                        // Execute the first command
                        execvp(argv1[0], argv1);

                        // If the first command was not successfully executed, print an error message and exit
                        printf("mysh: %s: command not found", argv1[0]);
                        exit(1);
                    }
                }
            }
        }
         else {
            printf("mysh: %s: command not found", argv[0]);
            exit_status = 1;
         }
    }
}

// Changes the current working directory of the shell program to argv[1] if it exists and is accessible, or prints an error message otherwise and sets exit_status to 1 if argv[1] is not specified or does not exist or is inaccessible; otherwise, sets exit_status to 0
void cd(char **argv) {
    // If argv[1] is not specified, print an error message and set exit_status to 1
    if (argv[1] == NULL) {
        printf("mysh: cd: missing operand");
        exit_status = 1;
    }

    // If argv[1] does not exist or is inaccessible, print an error message and set exit_status to 1
    else if (chdir(argv[1]) != 0) {
        printf("mysh: cd: %s: No such file or directory", argv[1]);
        exit_status = 1;
    }

    // Otherwise, set exit_status to 0
    else {
        exit_status = 0;
    }
}

// Prints the current working directory of the shell program to standard output and sets exit_status to 0
void pwd() {
    // Get the current working directory
    char cwd[256];
    getcwd(cwd, sizeof(cwd));

    // Print the current working directory
    printf("%s", cwd);

    // Set exit_status to 0
    exit_status = 0;
}

// Returns true if command is a built-in command, or false otherwise
bool is_builtin(char *command) {
    // If command is "cd", return true
    if (strcmp(command, "cd") == 0) {
        return true;
    }

    // If command is "pwd", return true
    if (strcmp(command, "pwd") == 0) {
        return true;
    }

    // Otherwise, return false
    return false;
}

// Returns true if command is an executable file in one of the directories in PATH, or false otherwise
bool is_executable(char *command) {
    // Get the value of the PATH environment variable
    char *path = getenv("PATH");

    // Parse the value of the PATH environment variable into an array of directories
    char *directories[256];
    int i = 0;
    char *token = strtok(path, ":");
    while (token != NULL) {
        directories[i] = token;
        token = strtok(NULL, ":");
        i++;
    }

    // For each directory in the array of directories, check if command is an executable file in that directory
    for (int i = 0; directories[i] != NULL; i++) {
        // Get the path to the directory
        char *directory = directories[i];

        // Get the path to the executable file
        char *executable = malloc(strlen(directory) + strlen(command) + 2);
        strcpy(executable, directory);
        strcat(executable, "/");
        strcat(executable, command);

        // If command is an executable file in the directory, return true
        if (access(executable, X_OK) == 0) {
            return true;
        }
    }

    // Otherwise, return false
    return false;
}

// Returns 1 if filename exists in dirname and can be executed, 0 otherwise; does not modify exit_status
int check_dir(char *dirname, char *filename) {
    // Open the directory
    DIR *dir = opendir(dirname);

    // If the directory was successfully opened, check if filename exists in the directory and can be executed
    if (dir != NULL) {
        // Read the directory
        struct dirent *entry;
        while ((entry = readdir(dir)) != NULL) {
            // If filename exists in the directory and can be executed, return 1
            if (strcmp(entry->d_name, filename) == 0 && access(entry->d_name, X_OK) == 0) {
                return 1;
            }
        }

        // Close the directory
        closedir(dir);
    }

    // Otherwise, return 0
    return 0;
}

 // Returns 1 if str1 contains str2 as a substring, 0 otherwise; does not modify exit_status
int contains(char *str1, char *str2) {
    // If str2 is a substring of str1, return 1
    if (strstr(str1, str2) != NULL) {
        return 1;
    }

    // Otherwise, return 0
    return 0;
}