# MyShell - Unix Shell Implementation Explained

## Overview
This document provides a deep dive into the implementation of `MyShell.c`, a Unix shell program that allows users to execute commands, manage background processes, and use built-in commands like `cd`, `echo`, and `export`.

## Features Implemented
- **Command Execution**: Runs commands with and without arguments.
- **Background Execution (`&`)**: Supports running processes in the background.
- **Built-in Commands**:
  - `exit`: Exits the shell.
  - `cd`: Changes the current working directory.
  - `echo`: Prints messages and expands variables.
  - `export`: Sets environment variables.
- **Process Management**: Handles child processes and prevents zombies using `SIGCHLD`.
- **Variable Expansion**: Supports `$VAR` substitution in commands.
- **Signal Handling**: Implements handlers for `SIGCHLD` and `SIGINT`.
- **Logging**: Logs terminated background processes.

---

## Code Breakdown

### 1. **Global Variables and Headers**
```c
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>
#include <string.h>
#include <signal.h>
#include <sys/wait.h>
#include <ctype.h>

#define MAX_INPUT 1024
#define MAX_ARGS 64
const char *LOG_FILE = "myshell.log";
```
- Includes necessary libraries for system calls (`fork`, `execvp`, `waitpid`), string manipulation, and signal handling.
- Defines maximum buffer sizes for input handling.
- `LOG_FILE` stores the name of the log file used for tracking terminated child processes.

---

### 2. **Main Function**
```c
int main()
{
    char input[MAX_INPUT];
    char *args[MAX_ARGS];
    char cwd[MAX_INPUT];

    struct sigaction sa;
    sa.sa_handler = sigchld_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART | SA_NOCLDSTOP;
    if (sigaction(SIGCHLD, &sa, 0) == -1)
    {
        perror("sigaction");
        exit(1);
    }

    char background_flag = 0;
```
- Initializes necessary variables for input storage and argument parsing.
- **Registers `SIGCHLD` signal handler** using `sigaction()`, which ensures that terminated child processes are properly handled.
- `SA_RESTART` allows interrupted system calls to resume, and `SA_NOCLDSTOP` prevents receiving signals for stopped child processes.

---

### 3. **Input Handling and Parsing**
```c
while (1)
{
    if (getcwd(cwd, sizeof(cwd)) != NULL)
    {
        printf("\033[1;34mMyShell: %s> \033[0m", cwd);
    }
    else
    {
        printf("\033[1;34mMyShell> \033[0m");
    }

    fflush(stdout);
    if (!fgets(input, MAX_INPUT, stdin))
        break;

    input[strcspn(input, "\n")] = 0;

    if (strlen(input) == 0)
        continue;
```
- **Displays the prompt**: Uses `getcwd()` to print the current directory.
- **Reads user input**: Uses `fgets()` to read a command from the user.
- **Removes newline characters**: `strcspn()` replaces `\n` with `\0`.
- **Skips empty input lines**.

---

### 4. **Handling Background Execution (`&`)**
```c
background_flag = 0;
if (input[strlen(input) - 1] == '&')
{
    background_flag = 1;
    input[strlen(input) - 1] = '\0';
    if (strlen(input) > 0 && input[strlen(input) - 1] == ' ')
        input[strlen(input) - 1] = '\0';
}
```
- **Detects `&`**: If the last character is `&`, sets `background_flag = 1`.
- **Removes unnecessary spaces** before `&`.

---

### 5. **Parsing Input into Arguments**
```c
parse_input(input, args);
if (args[0] == NULL)
    continue;
```
- Calls `parse_input()` to split user input into command and arguments.
- **Skips execution if the command is empty**.

```c
void parse_input(char *input, char *args[])
{
    int i = 0;
    char *token = strtok(input, " ");
    while (token)
    {
        args[i++] = token;
        token = strtok(NULL, " ");
    }
    args[i] = NULL;
}
```
- Uses `strtok()` to split the input into **tokens** based on spaces.
- Stores tokens in `args[]`, making it compatible with `execvp()`.

---

### 6. **Executing Commands**
```c
void execute_command(char *args[], char background_flag)
{
    pid_t pid = fork();

    if (pid < 0)
    {
        perror("fork");
        return;
    }
    else if (pid == 0)
    {
        if (execvp(args[0], args) == -1)
        {
            perror("execvp");
            exit(EXIT_FAILURE);
        }
    }
    else
    {
        if (!background_flag)
        {
            int status;
            waitpid(pid, &status, 0);
        }
        else
        {
            printf("[Background] Process ID: %d\n", pid);
        }
    }
}
```
- **Forks a new process**:
  - Parent process waits if not running in the background.
  - Child process replaces itself using `execvp()` to execute the command.
- **Error handling**:
  - `perror("execvp")` prints an error if the command is invalid.

---

### 7. **Handling Child Process Termination (`SIGCHLD`)**
```c
void sigchld_handler(int signo)
{
    int status;
    pid_t pid;
    while ((pid = waitpid(-1, &status, WNOHANG)) > 0)
    {
        FILE *log = fopen(LOG_FILE, "a");
        if (log)
        {
            fprintf(log, "Child process was terminated\n");
            fclose(log);
        }
    }
}
```
- **Handles terminated background processes** to prevent zombies.
- **Uses `waitpid()` with `WNOHANG`** to reap all terminated children **without blocking**.
- **Logs terminations** to a file.

---

### **Conclusion**
- This shell supports basic command execution, background processes, and built-in commands (`cd`, `echo`, `export`).
- `SIGCHLD` handling prevents zombie processes.
- Uses `execvp()` for executing external commands.
- Implements variable expansion for environment variables.

This document provides a detailed breakdown of the code and its logic. 🚀

