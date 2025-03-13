## **1. Mutex (`pthread_mutex_t`)**
A **mutex (mutual exclusion lock)** ensures that only **one thread** accesses a **critical section** at a time.

### **1.1 Mutex Functions**
| Function | Description |
|----------|------------|
| `pthread_mutex_init(pthread_mutex_t *mutex, const pthread_mutexattr_t *attr);` | Initializes a mutex. Use `NULL` for default attributes. |
| `pthread_mutex_destroy(pthread_mutex_t *mutex);` | Destroys a mutex after use. |
| `pthread_mutex_lock(pthread_mutex_t *mutex);` | Locks the mutex. If already locked, the calling thread blocks. |
| `pthread_mutex_trylock(pthread_mutex_t *mutex);` | Tries to lock the mutex, but **does not block** if it's already locked (returns `EBUSY`). |
| `pthread_mutex_timedlock(pthread_mutex_t *mutex, const struct timespec *timeout);` | Tries to lock the mutex, but **only waits up to the given time**. |
| `pthread_mutex_unlock(pthread_mutex_t *mutex);` | Unlocks the mutex. Must be called by the **same** thread that locked it. |

---

### **1.2 Mutex Example**
```c
#include <pthread.h>
#include <stdio.h>

pthread_mutex_t lock;

void *thread_func(void *arg) {
    pthread_mutex_lock(&lock);
    printf("Thread %ld is in the critical section\n", (long)arg);
    pthread_mutex_unlock(&lock);
    return NULL;
}

int main() {
    pthread_t t1, t2;
    pthread_mutex_init(&lock, NULL);
    
    pthread_create(&t1, NULL, thread_func, (void *)1);
    pthread_create(&t2, NULL, thread_func, (void *)2);

    pthread_join(t1, NULL);
    pthread_join(t2, NULL);

    pthread_mutex_destroy(&lock);
    return 0;
}
```

---

### **1.3 Mutex Types (`pthread_mutexattr_t`)**
Use `pthread_mutexattr_settype()` to set mutex behavior.

| Type | Description |
|------|------------|
| `PTHREAD_MUTEX_NORMAL` | Default, can cause **deadlocks** if re-locked by the same thread. |
| `PTHREAD_MUTEX_ERRORCHECK` | Detects errors like re-locking by the same thread. |
| `PTHREAD_MUTEX_RECURSIVE` | Allows the **same thread** to lock multiple times (must unlock the same number of times). |
| `PTHREAD_MUTEX_DEFAULT` | Same as `PTHREAD_MUTEX_NORMAL` on most systems. |

#### **Example: Initializing a Recursive Mutex**
```c
pthread_mutexattr_t attr;
pthread_mutexattr_init(&attr);
pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE);
pthread_mutex_init(&lock, &attr);
pthread_mutexattr_destroy(&attr);
```

---

## **2. Condition Variables (`pthread_cond_t`)**
Condition variables allow threads to **wait** until a condition is met.

### **2.1 Condition Variable Functions**
| Function | Description |
|----------|------------|
| `pthread_cond_init(pthread_cond_t *cond, const pthread_condattr_t *attr);` | Initializes a condition variable. Use `NULL` for default attributes. |
| `pthread_cond_destroy(pthread_cond_t *cond);` | Destroys a condition variable. |
| `pthread_cond_wait(pthread_cond_t *cond, pthread_mutex_t *mutex);` | Waits for a condition to be signaled. **Releases the mutex while waiting.** |
| `pthread_cond_timedwait(pthread_cond_t *cond, pthread_mutex_t *mutex, const struct timespec *timeout);` | Waits for a condition with a timeout. |
| `pthread_cond_signal(pthread_cond_t *cond);` | Wakes up **one** thread waiting on the condition. |
| `pthread_cond_broadcast(pthread_cond_t *cond);` | Wakes up **all** threads waiting on the condition. |

---

### **2.2 Condition Variable Example**
```c
#include <pthread.h>
#include <stdio.h>

pthread_mutex_t lock;
pthread_cond_t cond;
int ready = 0;

void *thread_func(void *arg) {
    pthread_mutex_lock(&lock);
    while (!ready) {
        pthread_cond_wait(&cond, &lock);
    }
    printf("Thread %ld received signal!\n", (long)arg);
    pthread_mutex_unlock(&lock);
    return NULL;
}

int main() {
    pthread_t t1, t2;
    pthread_mutex_init(&lock, NULL);
    pthread_cond_init(&cond, NULL);

    pthread_create(&t1, NULL, thread_func, (void *)1);
    pthread_create(&t2, NULL, thread_func, (void *)2);

    sleep(2); // Simulate work

    pthread_mutex_lock(&lock);
    ready = 1;
    pthread_cond_broadcast(&cond);
    pthread_mutex_unlock(&lock);

    pthread_join(t1, NULL);
    pthread_join(t2, NULL);

    pthread_mutex_destroy(&lock);
    pthread_cond_destroy(&cond);
    return 0;
}
```

---

## **3. Common Pitfalls**
❌ **Forgetting to unlock the mutex**  
✅ Always call `pthread_mutex_unlock()` after `pthread_mutex_lock()`.

❌ **Using `pthread_cond_wait()` without a mutex**  
✅ `pthread_cond_wait()` requires a **locked mutex**, and it automatically releases the mutex while waiting.

❌ **Not handling spurious wakeups**  
✅ Always use a **while-loop** for condition checking in `pthread_cond_wait()`.

❌ **Destroying mutex/condition while in use**  
✅ Ensure no thread is using them before calling `pthread_mutex_destroy()` or `pthread_cond_destroy()`.

---

## **4. Summary Table**
| Feature | `pthread_mutex_t` | `pthread_cond_t` |
|---------|------------------|------------------|
| Purpose | Mutual exclusion for threads | Synchronization of threads using conditions |
| Requires Mutex? | No | Yes (must be used with a mutex) |
| Blocking Function | `pthread_mutex_lock()` | `pthread_cond_wait()` |
| Non-blocking Function | `pthread_mutex_trylock()` | N/A |
| Wake-up Mechanism | `pthread_mutex_unlock()` | `pthread_cond_signal()` or `pthread_cond_broadcast()` |
| Timeout Support | `pthread_mutex_timedlock()` | `pthread_cond_timedwait()` |

---

## **5. Sources**
- [What are race conditions?](https://youtu.be/FY9livorrJI)
- [What is Mutex?](https://youtu.be/oq29KUy29iQ)
- [Condition variable](https://youtu.be/0sVGnxg6Z3k?feature=shared)
- [Signalling for condition variables](https://youtu.be/RtTlIvnBw10?feature=shared)