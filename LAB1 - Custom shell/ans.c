// Libraries
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>
#include <string.h>
#include <signal.h>
#include <sys/wait.h>
#include <ctype.h>

// Global Variables
#define MAX_INPUT 1024
#define MAX_ARGS 64

// Functions prototypes
void parse_input(char *input, char *args[]);
void execute_command(char *args[], int background_flag);
void sigchld_handler(int signo);

void handle_cd(char *args[]);
void handle_echo(char *args[]);
void handle_export(char *args[]);
void expand_variables(char *str);

// Log file name
const char* LOG_FILE = "myshell.log";

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
    if (sigaction(SIGCHLD, &sa, 0) == -1) {
        perror("sigaction");
        exit(1);
    }

    // Background process flag
    int background_flag = 0;

    while (1)
    {
        // Get the current working directory
        if (getcwd(cwd, sizeof(cwd)) != NULL)
        {
            printf("MyShell:%s> ", cwd);
        }
        else
        {
            printf("MyShell> ");
        }
        fflush(stdout);

        // EOF encountered (ctrl+D) or max input exceeded -> BREAK
        if (!fgets(input, MAX_INPUT, stdin))
            break;

        // Format input by replacing \n to \0
        input[strcspn(input, "\n")] = 0;

        // Skips empty lines
        if (strlen(input) == 0)
            continue;

        // Dealing with background processes
        background_flag = 0;
        if (input[strlen(input) - 1] == '&')
        {
            background_flag = 1;
            input[strlen(input) - 1] = '\0';
            // Remove any space before &
            if (strlen(input) > 0 && input[strlen(input) - 1] == ' ')
                input[strlen(input) - 1] = '\0';
        }

        // Parse input into arguments
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

    printf("Exiting MyShell...\n");
    return 0;
}

// A function to parse the input
void parse_input(char *input, char *args[])
{
    int i = 0;
    char *token = strtok(input, " \t"); // Split on spaces and tabs
    
    while (token != NULL)
    {
        args[i++] = token;
        token = strtok(NULL, " \t");
    }
    args[i] = NULL; // Mark the end of arguments with NULL
}

// Function to execute commands
void execute_command(char *args[], int background_flag)
{
    // Create a new child process
    pid_t pid = fork();

    if (pid < 0)
    {
        perror("fork");
        return;
    }
    else if (pid == 0) // Child process
    {
        // Execute the command
        if (execvp(args[0], args) == -1)
        {
            perror("execvp");
            exit(EXIT_FAILURE);
        }
    }
    else // Parent process
    {
        if (!background_flag)
        {
            // Wait for the child process to complete
            int status;
            waitpid(pid, &status, 0);
            
            // Check if process exited abnormally
            if (WIFEXITED(status) && WEXITSTATUS(status) != 0)
            {
                printf("Command exited with non-zero status %d\n", WEXITSTATUS(status));
            }
        }
        else
        {
            // For background processes, just print the PID
            printf("[%d] started in background\n", pid);
        }
    }
}

// Signal handler for SIGCHLD
void sigchld_handler(int signo)
{
    // Variables to store status and pid
    int status;
    pid_t pid;
    
    // Reap all terminated children
    while ((pid = waitpid(-1, &status, WNOHANG)) > 0)
    {
        // Write to log file
        FILE *log = fopen(LOG_FILE, "a");
        if (log)
        {
            fprintf(log, "Child process was terminated\n");
            fclose(log);
        }
    }
}

// cd command to change the current working directory
void handle_cd(char *args[])
{
    char *target_dir;
    
    // Case 1: cd with no args or cd ~
    if (args[1] == NULL || strcmp(args[1], "~") == 0)
    {
        target_dir = getenv("HOME");
        if (target_dir == NULL)
        {
            fprintf(stderr, "cd: HOME not set\n");
            return;
        }
    }
    // Case 2: cd ..
    else if (strcmp(args[1], "..") == 0)
    {
        target_dir = args[1];
    }
    // Case 3: cd to absolute or relative path
    else
    {
        target_dir = args[1];
    }
    
    // Change directory
    if (chdir(target_dir) != 0)
    {
        perror("cd");
    }
}

// Function to recreate the original string from args
char* reconstruct_args(char *args[], int start_idx)
{
    static char buffer[MAX_INPUT];
    buffer[0] = '\0';
    
    int i = start_idx;
    while (args[i] != NULL)
    {
        strcat(buffer, args[i]);
        if (args[i+1] != NULL)
            strcat(buffer, " ");
        i++;
    }
    
    return buffer;
}

// echo command to print arguments to the console
void handle_echo(char *args[])
{
    // If no arguments, just print a newline
    if (args[1] == NULL) {
        printf("\n");
        return;
    }
    
    int i = 1;
    while (args[i] != NULL) {
        char expanded[MAX_INPUT];
        strcpy(expanded, args[i]);
        
        // Expand variables in the argument
        if (strchr(expanded, '$') != NULL) {
            expand_variables(expanded);
        }
        
        // If this is a quoted string, remove the quotes
        int len = strlen(expanded);
        if (len >= 2 && expanded[0] == '"' && expanded[len-1] == '"') {
            // Remove quotes and print the content
            expanded[len-1] = '\0';
            printf("%s", expanded + 1);
        } else {
            printf("%s", expanded);
        }
        
        // Add space between arguments
        if (args[i+1] != NULL) {
            printf(" ");
        }
        i++;
    }
    printf("\n");
}

// export command to set environment variables
void handle_export(char *args[])
{
    // Check if there's an argument to export
    if (args[1] == NULL) {
        fprintf(stderr, "export: missing variable assignment\n");
        return;
    }
    
    // Locate the equals sign to separate variable name and value
    char *equals_pos = strchr(args[1], '=');
    if (equals_pos == NULL) {
        fprintf(stderr, "export: invalid syntax\n");
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

    
    // Check if this is a quoted string
    if (*value_start == '"') {
        // Skip the opening quote
        value_start++;
        
        // Check if the closing quote is in the same argument
        char *closing_quote = strchr(value_start, '"');
        if (closing_quote != NULL) {
            // Copy everything between quotes
            strncpy(value, value_start, closing_quote - value_start);
            value[closing_quote - value_start] = '\0';
        } else {
            // The string continues to other arguments
            strcpy(value, value_start);
            
            // Process remaining arguments to find closing quote
            int i = 2;
            while (args[i] != NULL) {
                strcat(value, " ");  // Add space between arguments
                
                char *arg_content = args[i];
                closing_quote = strchr(arg_content, '"');
                
                if (closing_quote != NULL) {
                    // Found closing quote, copy up to it
                    strncat(value, arg_content, closing_quote - arg_content);
                    break;
                } else {
                    // This argument is part of the value
                    strcat(value, arg_content);
                }
                i++;
            }
        }
    } else {
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