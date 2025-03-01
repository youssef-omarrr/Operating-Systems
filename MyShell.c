// Libraries
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>
#include <string.h>
#include <signal.h>
#include <sys/wait.h>
#include <ctype.h>

/*
    To build:
    gcc MyShell.c -o MyShell

    To run:
    ./MyShell

    To run System Monito:
    gnome-system-monitor &

*/

// Global Variables
#define MAX_INPUT 1024
#define MAX_ARGS 64

// Functions
void parse_input(char *input, char *args[]);
void execute_command(char *args[], char background_flag);
void sigchld_handler(int signo);
char *reconstruct_args(char *args[], int start_idx);

void handle_cd(char *args[]);
void handle_echo(char *args[]);
void handle_export(char *args[]);
void expand_variables(char *str);

// Log file name
const char *LOG_FILE = "myshell.log";

int main()
{
    // Variables to store input
    char input[MAX_INPUT];
    char *args[MAX_ARGS];
    char cwd[MAX_INPUT]; // Buffer to store the current working directory

    // Set up signal handling with sigaction for SIGCHLD
    struct sigaction sa;
    sa.sa_handler = sigchld_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART | SA_NOCLDSTOP;
    if (sigaction(SIGCHLD, &sa, 0) == -1)
    {
        perror("sigaction");
        exit(1);
    }

    // Background process flag
    char background_flag = 0;

    while (1)
    {

        // Get the current working directory
        if (getcwd(cwd, sizeof(cwd)) != NULL)
        {
            // Update the prompt to include the path
            printf("\033[1;34mMyShell: %s> \033[0m", cwd);
        }
        else
        {
            // Fallback to the default prompt if getcwd() fails
            printf("\033[1;34mMyShell> \033[0m");
        }

        fflush(stdout);

        // EOF encountered (ctrl+D) or max input exceded -> BREAK
        if (!fgets(input, MAX_INPUT, stdin))
            break;

        // Format input by replacing \n to \0
        input[strcspn(input, "\n")] = 0;

        // Skips empty lines
        if (strlen(input) == 0)
            continue;

        // Dealing with background processes (intially = 0)
        background_flag = 0;
        if (input[strlen(input) - 1] == '&')
        {
            background_flag = 1;
            input[strlen(input) - 1] = '\0';

            // Remove any space before &
            if (strlen(input) > 0 && input[strlen(input) - 1] == ' ')
                input[strlen(input) - 1] = '\0';
        }

        // Parse input into (arg[0] = command , arg[n] = arguments)
        parse_input(input, args);
        if (args[0] == NULL)
            continue;

        // Handle built-in commands
        if (strcmp(args[0], "exit") == 0)
            break;

        else if (strcmp(args[0], "cd") == 0)
        {
            handle_cd(args);
            continue;
        }
        else if (strcmp(args[0], "echo") == 0)
        {
            handle_echo(args);
            continue;
        }
        else if (strcmp(args[0], "export") == 0)
        {
            handle_export(args);
            continue;
        }

        // Expand variables in arguments
        for (int i = 0; args[i] != NULL; i++)
        {
            if (strchr(args[i], '$') != NULL)
                expand_variables(args[i]);
        }

        // Execute external commands
        execute_command(args, background_flag);
    }

    printf("\033[1;36mExiting MyShell...\033[0m\n");
    return 0;
}

/////////////////////////////////////////////////////////////////////
//////////////**********  FUNCTIONS   ***********////////////////////
/////////////////////////////////////////////////////////////////////

// A function to parse the input in this format (arg[0] = command , arg[n] = arguments)
void parse_input(char *input, char *args[])
{
    int i = 0;
    char *token = strtok(input, " "); // strtok() splits input into tokens based on spaces.
    while (token)
    {
        args[i++] = token;
        token = strtok(NULL, " ");
    }
    args[i] = NULL;
}

// Function to execute commands
void execute_command(char *args[], char background_flag)
{
    // Create a new child process (parent is the shell)
    pid_t pid = fork();

    /*
    If fork() returns:
        < 0  : Forking failed, so an error is printed using perror("fork").
        == 0 : This is the child process.
        > 0  : This is the parent process.
    */

    if (pid < 0)
    {
        perror("fork");
        return;
    }

    // Child process
    else if (pid == 0)
    {
        // The child process replaces itself with the requested command using execvp().
        if (execvp(args[0], args) == -1)
        {
            perror("execvp");
            // ensure that if execution fails, the child process terminates cleanly.
            exit(EXIT_FAILURE);
        }
    }

    // Parent process
    else
    {
        if (!background_flag)
        {
            // The parent waits for the child process to complete. (if backgorund == 0)
            int status;
            waitpid(pid, &status, 0);
            if (WIFEXITED(status) && WEXITSTATUS(status) != 0)
            {
                printf("\033[1;31mAbnormal exit: %d\033[0m\n", WEXITSTATUS(status));
            }
        }
        else
        {
            // The parent does not wait, allowing the shell to continue accepting new commands.
            printf("[Background] Process ID: %d\n", pid);
        }
    }
}

// Signal handler for SIGCHLD (sent to the parent process whenever a child process terminates.)
void sigchld_handler(int signo)
{
    // Variables to store status and pid
    int status; // Stores the exit status of the terminated child process.
    pid_t pid;  // Stores the process ID (PID) of the child process that has terminated.

    // Reap (clean up) all terminated children
    // The while loop ensures that all zombie processes are reaped
    while ((pid = waitpid(-1, &status, WNOHANG)) > 0)
    {
        /*
        pid = waitpid(-1, &status, WNOHANG

        -1 : Means "wait for any child process."
        &status : Stores the exit status of the child.
        WNOHANG :  Ensures non-blocking behavior:

        If no child has terminated, waitpid() returns 0 instead of blocking the shell.
        If a child has terminated, waitpid() returns its PID.
        */

        // Open a log file (LOG_FILE) in append mode ("a") to record that a child process has exited.
        FILE *log = fopen(LOG_FILE, "a");
        if (log) // Checks if the file was opened successfully.
        {
            fprintf(log, "Child process was terminated\n");
            fclose(log);
        }
    }
}

// cd command to change the current working directory.
void handle_cd(char *args[])
{
    char *target_dir;

    // Case 1: cd (no arguments) or cd ~ -> Change to home directory
    if (!args[1] || strcmp(args[1], "~") == 0)
    {
        target_dir = getenv("HOME"); // Get the home directory from environment variables
        if (!target_dir)
        {
            fprintf(stderr, "\033[1;31mcd: HOME environment variable not set\033[0m\n");
            return;
        }
    }

    // Case 2: cd ..-> Change to parent directory
    else if (strcmp(args[1], "..") == 0)
    {
        target_dir = args[1];
    }

    // Case 3: cd absolute_path or cd relative_path -> Change to the specified path
    // Handle paths with spaces by concatenating multiple arguments

    else if ((args[1][0] == '"') || (args[1][0] == '\''))
    {
        char path[1024] = "";
        int i = 1;
        char quote_char = args[1][0]; // Store quote type (' or ")

        while (args[i])
        {
            strcat(path, args[i]);
            if (args[i + 1]) // Add space between words if more arguments exist
                strcat(path, " ");
            i++;
        }

        // Remove leading and trailing quotes
        int len = strlen(path);
        if (len > 1 && path[len - 1] == quote_char)
        {
            path[len - 1] = '\0'; // Remove the closing quote
        }
        memmove(path, path + 1, strlen(path)); // Remove the first quote

        strcpy(target_dir, path);
    }

    else
    {
        // If no quotes, use only the first argument
        strcpy(target_dir, args[1]);
    }

    // Attempt to change the directory
    if (chdir(target_dir) != 0)
    {
        fprintf(stderr, "\033[1;31mcd: %s: No such file or directory\033[0m\n", target_dir);
    }
}

// echo command to print arguments to the console.
void handle_echo(char *args[])
{

    // If no arguments, just print a newline
    if (args[1] == NULL)
    {
        printf("\n");
        return;
    }

    int i = 1;
    while (args[i] != NULL)
    {
        char expanded[MAX_INPUT];
        strcpy(expanded, args[i]);

        // Expand variables in the argument
        if (strchr(expanded, '$') != NULL)
        {
            expand_variables(expanded);
        }

        // If this is a quoted string, remove the quotes
        int len = strlen(expanded);
        if (len >= 2 && expanded[0] == '"' && expanded[len - 1] == '"')
        {
            // Remove quotes and print the content
            expanded[len - 1] = '\0';
            printf("%s", expanded + 1);
        }
        else
        {
            printf("%s", expanded);
        }

        // Add space between arguments
        if (args[i + 1] != NULL)
        {
            printf(" ");
        }
        i++;
    }

    printf("\n");
}

// export command to set environment variables.
void handle_export(char *args[])
{
    if (!args[1])
    {
        fprintf(stderr, "\033[1;31mexport: missing argument\033[0m\n");
        return;
    }

    // Locate the '=' sign in args[1] to separate the variable name
    char *equals_pos = strchr(args[1], '=');
    if (!equals_pos)
    {
        fprintf(stderr, "\033[1;31mexport: invalid syntax. Usage: export VAR=value\033[0m\n");
        return;
    }

    // Extract the variable name (before the equals sign)
    char var_name[MAX_INPUT];
    int name_len = equals_pos - args[1]; // Length of the variable name
    strncpy(var_name, args[1], name_len);
    var_name[name_len] = '\0'; // Null-terminate the name

    // Get the value part (after the equals sign)
    char *value_start = equals_pos + 1; // Start of the value
    char value[MAX_INPUT] = "";         // Buffer for the full value

    // If the value starts with a quote, extract the entire quoted string
    if (*value_start == '"')
    {
        // Skip the opening quote
        value_start++;

        // Check if the closing quote is in the same argument
        char *closing_quote = strchr(value_start, '"');
        if (closing_quote != NULL)
        {
            // Copy everything between quotes
            strncpy(value, value_start, closing_quote - value_start);
            value[closing_quote - value_start] = '\0';
        }
        else
        {
            // The string continues to other arguments
            strcpy(value, value_start);

            // Process remaining arguments to find closing quote
            int i = 2;
            while (args[i] != NULL)
            {
                strcat(value, " "); // Add space between arguments

                char *arg_content = args[i];
                closing_quote = strchr(arg_content, '"');

                if (closing_quote != NULL)
                {
                    // Found closing quote, copy up to it
                    strncat(value, arg_content, closing_quote - arg_content);
                    break;
                }
                else
                {
                    // This argument is part of the value
                    strcat(value, arg_content);
                }
                i++;
            }
        }
    }
    else
    {
        // Not quoted, just copy the value
        strcpy(value, value_start);
    }

    // Set the environment variable
    setenv(var_name, value, 1);
}

// Improve variable expansion to handle adjacent variables
void expand_variables(char *str)
{
    char result[MAX_INPUT] = "";
    char temp[MAX_INPUT] = "";
    char *read_ptr = str;
    
    while (*read_ptr) {
        if (*read_ptr == '$') {
            // Variable found, skip the $
            read_ptr++;
            
            // Extract the variable name
            char var_name[MAX_INPUT] = "";
            int i = 0;
            
            while (isalnum(*read_ptr) || *read_ptr == '_') {
                var_name[i++] = *read_ptr++;
            }
            var_name[i] = '\0';
            
            // Get the value and append it to the result
            char *value = getenv(var_name);
            if (value) {
                strcat(result, value);
            }
        } else {
            // Append the current character
            temp[0] = *read_ptr;
            temp[1] = '\0';
            strcat(result, temp);
            read_ptr++;
        }
    }
    
    // Copy the result back to the original string
    strcpy(str, result);
}

// Function to recreate the original string from args
char *reconstruct_args(char *args[], int start_idx)
{
    static char buffer[MAX_INPUT];
    buffer[0] = '\0';

    int i = start_idx;
    while (args[i] != NULL)
    {
        strcat(buffer, args[i]);
        if (args[i + 1] != NULL)
            strcat(buffer, " ");
        i++;
    }

    return buffer;
}