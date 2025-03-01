# MyShell - Unix Shell Implementation Explained

# Overview
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

# Code Breakdown

## 1. **Global Variables and Headers**
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

## 2. **signal handling**
```c
    struct sigaction sa;
    sa.sa_handler = sigchld_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART | SA_NOCLDSTOP;
    if (sigaction(SIGCHLD, &sa, 0) == -1)
    {
        perror("sigaction");
        exit(1);
    }
```
- **Declares a sigaction struct (sa) to specify the behavior of the SIGCHLD signal.**

- **Assigns the signal handler function:**
  - **sa.sa_handler = sigchld_handler;** → **Calls** sigchld_handler when SIGCHLD is received.

- **Initializes the signal mask:**
  - **sigemptyset(&sa.sa_mask);** → **Clears** all **blocked** signals to ensure no other signals interfere.

- **Sets signal handler flags:**
  - **SA_RESTART:** Ensures **interrupted** system calls (e.g., read, waitpid) **resume execution.**
  - **SA_NOCLDSTOP:** Prevents **receiving** SIGCHLD signals when a child process is **stopped** (only triggers on termination).

- **Registers the signal handler using sigaction(SIGCHLD, &sa, 0);.**
  - If **sigaction()** fails, it prints an error message and exits the shell.

---

## 3. **Input Handling and Parsing**
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

## 4. **Handling Background Execution (`&`)**
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

## 5. Understanding `parse_input()` Function
```c
void parse_input(char *input, char *args[])
{
    int i = 0;
    char *token = strtok(input, " "); // strtok() splits input into tokens based on spaces.
    while (token)
    {
        args[i++] = token; // Store token in args[]
        token = strtok(NULL, " "); // Get next token
    }
    args[i] = NULL; // Null-terminate the argument list
}
```

---

### **Step-by-Step Breakdown**

### **5.1. Function Parameters**
```c
void parse_input(char *input, char *args[])
```
- **`char *input`** → The full user input string (e.g., `"ls -l /home"`).
- **`char *args[]`** → An array that will store the command and its arguments (e.g., `["ls", "-l", "/home", NULL]`).

---

### **5.2. First Call to `strtok()`**
```c
char *token = strtok(input, " ");
```
- `strtok(input, " ")` **splits `input` at the first space**.
- Returns a **pointer** to the first token (the command).
- **Modifies `input` in-place** by replacing the space with `\0` (null terminator).

#### **Example: Tokenizing `"ls -l /home"`**
| Input String Before | After `strtok(input, " ")` | Token |
|---------------------|---------------------------|-------|
| `"ls -l /home"`    | `"ls\0-l /home"`          | `"ls"` |

---

### **5.3. Loop Through Remaining Tokens**
```c
while (token)
{
    args[i++] = token; // Store token in args[]
    token = strtok(NULL, " "); // Get next token
}
```
- `strtok(NULL, " ")` finds the **next token**.
- Stores tokens in `args[]`, making it compatible with `execvp()`.

#### **Example Execution**
| Iteration | Token  | args[i]  |
|-----------|--------|----------|
| 1st       | `"ls"` | `args[0] = "ls"` |
| 2nd       | `"-l"` | `args[1] = "-l"` |
| 3rd       | `"/home"` | `args[2] = "/home"` |
| 4th       | `NULL` | `args[3] = NULL` (loop exits) |

---

### **5.4. Null-Terminate the Argument List**
```c
args[i] = NULL;
```
- Ensures the last element is `NULL` (required for `execvp()`).

---

### **Full Example Execution**

### **Expected Output**
```
char input[] = "ls -l /home";

Parsed Arguments:
args[0]: ls
args[1]: -l
args[2]: /home
```

---

### **Key Takeaways**
- **Splits input into words** using `strtok()`.
- **Modifies `input` directly**, replacing spaces with `\0`.
- **Stores tokens in `args[]`**, with the last element set to `NULL`.
- **Used before `execvp()`**, which expects `args[]` as input.

---

## 6. **Executing Commands**
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
### Explanation of `execute_command` Function

The `execute_command` function is responsible for executing a command in a child process using the `fork()` and `execvp()` system calls. The command can be run in either foreground or background mode, depending on the `background_flag`.

---

### **Function Signature**
```c
void execute_command(char *args[], char background_flag)
```
- **Parameters:**
  - `char *args[]`: An array of strings representing the command and its arguments (e.g., `{"ls", "-l", NULL}`).
  - `char background_flag`: A flag that determines whether the command runs in the background (`1`) or foreground (`0`).

---

## **6.1. Forking a New Process**
```c
pid_t pid = fork();
```
- The `fork()` system call creates a new child process.
- The return value of `fork()`:
  - `< 0`: Fork failed.
  - `== 0`: This is the child process.
  - `> 0`: This is the parent process.

---

## **6.2. Handling Fork Errors**
```c
if (pid < 0)
{
    perror("fork");
    return;
}
```
- If `fork()` fails, `perror("fork")` prints an error message, and the function returns.

---

## **6.3. Executing the Command in the Child Process**
```c
else if (pid == 0)
{
    if (execvp(args[0], args) == -1)
    {
        perror("execvp");
        exit(EXIT_FAILURE);
    }
}
```
- If `pid == 0`, the process is the child.
- `execvp(args[0], args)`: Replaces the child process with the command specified in `args`.
  - `args[0]`: The command (e.g., `"ls"`).
  - `args`: The full argument list (e.g., `{"ls", "-l", NULL}`).
- If `execvp` fails, an error message is printed, and the process exits with `EXIT_FAILURE`.

---

## **6.4. Parent Process Handling**
```c
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
```
- **If `background_flag == 0` (Foreground Execution)**:
  - `waitpid(pid, &status, 0);` waits for the child process to complete before continuing.
- **If `background_flag == 1` (Background Execution)**:
  - The parent does not wait and instead prints the child's process ID.

---

## **Example Usage**
### **1. Running a Foreground Command**
```c
char *args[] = {"ls", "-l", NULL};
execute_command(args, 0);
```
- Runs `ls -l` and waits for it to finish.

### **2. Running a Background Command**
```c
char *args[] = {"sleep", "10", NULL};
execute_command(args, 1);
```
- Runs `sleep 10` in the background and immediately returns.

---

## **Summary**
- `fork()` creates a new child process.
- In the child process:
  - `execvp()` executes the command.
- In the parent process:
  - If `background_flag == 0`, the parent waits for the child.
  - If `background_flag == 1`, the parent continues running while the child process executes in the background.

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
- **Uses `waitpid()` with `WNOHANG`** to reap (clean up) all terminated children **without blocking**.
- **Logs terminations** to a file.
   
```c
pid = waitpid(-1, &status, WNOHANG)
```
- **-1 :** Means "**wait** for any child process."
- **&status :** Stores the **exit status** of the child.
- **WNOHANG :**  Ensures **non-blocking** behavior:

- If **no child has terminated**, waitpid() returns 0 instead of blocking the shell.
- If a **child has terminated**, waitpid() returns its PID.

---

### **Conclusion**
- This shell supports basic command execution, background processes, and built-in commands (`cd`, `echo`, `export`).
- `SIGCHLD` handling prevents zombie processes.
- Uses `execvp()` for executing external commands.
- Implements variable expansion for environment variables.

