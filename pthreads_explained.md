# Pthreads Library in C: Main Functions Guide

The POSIX Threads (pthreads) library provides a set of functions for creating and managing threads in C. This guide covers the main functions used for thread management, synchronization, and attributes.

**Header File:**  
```c
#include <pthread.h>
```

**Compilation:**  
Compile with `-lpthread` flag:
```bash
gcc program.c -o program -lpthread
```

---

## Thread Management Functions

### 1. `pthread_create()`
Creates a new thread.

**Syntax:**
```c
int pthread_create(pthread_t *thread_id, const pthread_attr_t *attr,
                   void *(*start_routine)(void *), void *arg);
```

- **Parameters:**
  - `thread_id`: Pointer to store the thread ID.
  - `attr`: Thread attributes (NULL for defaults).
  - `start_routine`: Function the thread will execute.
  - `arg`: Argument passed to `start_routine`.

**Return Value:**  
0 on success, error code on failure.

**Example:**
```c
void *print_message(void *msg) {
    printf("%s\n", (char *)msg);
    return NULL;
}

pthread_t tid;
char *message = "Hello from thread!";
pthread_create(&tid, NULL, print_message, message);
```

---

### 2. `pthread_join()`
Waits for a thread to terminate and retrieves its exit status.

**Syntax:**
```c
int pthread_join(pthread_t thread_id, void **exit_status);
```

- **Parameters:**
  - `thread_id`: Thread ID to wait for.
  - `exit_status`: Pointer to store the thread's exit status.

**Return Value:**  
0 on success, error code on failure.

**Example:**
```c
void *result;
pthread_join(tid, &result); // Wait for 'tid' to finish
```

---

### 3. `pthread_exit()`
Terminates the calling thread and returns a value.

**Syntax:**
```c
void pthread_exit(void *exit_status);
```

- **Parameters:**
  - `exit_status`: Exit status (accessible via `pthread_join`).

**Example:**
```c
void *thread_func(void *arg) {
    pthread_exit((void *)42); // Exit with status 42
}
```

---

### 4. `pthread_detach()**
Marks a thread as detached, so its resources are automatically released.

**Syntax:**
```c
int pthread_detach(pthread_t thread);
```

**Return Value:**  
0 on success, error code on failure.

**Example:**
```c
pthread_detach(tid); // Thread 'tid' will auto-cleanup
```

---

### 5. `pthread_self()`
Returns the thread ID of the calling thread.

**Syntax:**
```c
pthread_t pthread_self(void);
```

**Example:**
```c
pthread_t my_tid = pthread_self();
```

---

### 6. `pthread_equal()`
Compares two thread IDs.

**Syntax:**
```c
int pthread_equal(pthread_t t1, pthread_t t2);
```

**Return Value:**  
Non-zero if equal, 0 otherwise.

**Example:**
```c
if (pthread_equal(tid1, tid2)) {
    printf("Thread IDs are equal.\n");
}
```

---

### 7. `pthread_cancel()`
Requests cancellation of a thread.

**Syntax:**
```c
int pthread_cancel(pthread_t thread);
```

**Return Value:**  
0 on success, error code on failure.

**Example:**
```c
pthread_cancel(tid); // Request cancellation of 'tid'
```

---

## Synchronization Functions

### 1. Mutex Functions
Manage mutual exclusion locks.

- **Create/Destroy:**
  ```c
  int pthread_mutex_init(pthread_mutex_t *mutex, const pthread_mutexattr_t *attr);
  int pthread_mutex_destroy(pthread_mutex_t *mutex);
  ```

- **Lock/Unlock:**
  ```c
  int pthread_mutex_lock(pthread_mutex_t *mutex);
  int pthread_mutex_unlock(pthread_mutex_t *mutex);
  ```

**Example:**
```c
pthread_mutex_t mutex;
pthread_mutex_init(&mutex, NULL);

pthread_mutex_lock(&mutex);
// Critical section
pthread_mutex_unlock(&mutex);
```

---

### 2. Condition Variables
Coordinate threads based on conditions.

- **Create/Destroy:**
  ```c
  int pthread_cond_init(pthread_cond_t *cond, const pthread_condattr_t *attr);
  int pthread_cond_destroy(pthread_cond_t *cond);
  ```

- **Wait/Signal:**
  ```c
  int pthread_cond_wait(pthread_cond_t *cond, pthread_mutex_t *mutex);
  int pthread_cond_signal(pthread_cond_t *cond);
  int pthread_cond_broadcast(pthread_cond_t *cond);
  ```

**Example:**
```c
pthread_cond_t cond;
pthread_mutex_t mutex;
// Thread 1:
pthread_mutex_lock(&mutex);
pthread_cond_wait(&cond, &mutex); // Wait for signal
pthread_mutex_unlock(&mutex);

// Thread 2:
pthread_cond_signal(&cond); // Wake up waiting thread
```

---

## Thread Attributes

### 1. `pthread_attr_init()`
Initialize thread attributes.

**Syntax:**
```c
int pthread_attr_init(pthread_attr_t *attr);
```

### 2. `pthread_attr_setdetachstate()`
Set detach state (e.g., `PTHREAD_CREATE_DETACHED`).

**Syntax:**
```c
int pthread_attr_setdetachstate(pthread_attr_t *attr, int detachstate);
```

### 3. `pthread_attr_destroy()`
Destroy thread attributes.

**Syntax:**
```c
int pthread_attr_destroy(pthread_attr_t *attr);
```

**Example:**
```c
pthread_attr_t attr;
pthread_attr_init(&attr);
pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
pthread_t tid;
pthread_create(&tid, &attr, thread_func, NULL);
pthread_attr_destroy(&attr);
```

---

## Key Points

- Always check return values for error handling.
- Use mutexes to protect shared data from race conditions.
- Detached threads cannot be joined; ensure proper synchronization.
- Condition variables must be used with a mutex for atomic operations.
