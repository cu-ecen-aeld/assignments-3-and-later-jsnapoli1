# Assignment 4 Part 1 Code Review

This review documents each pseudocode block added to the threading implementation, why it was structured the way it is, and how the code maps to the assignment requirements. It is intended to be a readable engineering review for a junior developer.

## Pseudocode Block 1: `struct thread_data` layout (threading.h)

```c
/*
 * PSEUDOCODE:
 *   Store the time to wait before obtaining the mutex.
 *   Store the time to hold the mutex before releasing it.
 *   Store the mutex pointer for lock/unlock operations.
 *   Store a success flag to report completion to the joiner.
 */
```

### Rationale and methodology
- **Why this layout:** The thread needs three runtime inputs (two wait durations and the mutex handle) plus a completion flag to report success. These map directly to the requirements in the assignment description, which specifies both wait durations, the mutex, and a success indicator.
- **Architecture decision:** Using a single `struct thread_data` as a data contract keeps thread configuration consistent and keeps thread lifetime management clear (the struct is allocated by the creator and returned by the thread). This is a common pattern in POSIX threading when you need to pass multiple parameters into a thread entry point.
- **Clarity:** Explicitly naming the fields as `wait_to_obtain_ms` and `wait_to_release_ms` keeps the time units obvious and minimizes confusion compared to generic names.

## Pseudocode Block 2: `threadfunc` behavior (threading.c)

```c
// PSEUDOCODE:
//   Cast thread_param into a struct thread_data pointer.
//   Sleep for wait_to_obtain_ms to simulate pre-lock work.
//   Lock the mutex and bail out on error.
//   Sleep for wait_to_release_ms while holding the mutex.
//   Unlock the mutex and mark the thread as successful.
```

### Rationale and methodology
- **Casting the argument:** `pthread_create` passes a `void*` to the thread function, so casting it to `struct thread_data*` is required to access fields. The pseudocode explicitly calls this out because it is easy to forget for developers unfamiliar with pthread APIs.
- **Sleeping in milliseconds:** `usleep` uses microseconds, so the code multiplies millisecond input by `1000` and casts to `useconds_t` to avoid truncation warnings. This makes the timing behavior match the assignment spec. See `man usleep` for the interface: https://man7.org/linux/man-pages/man3/usleep.3.html.
- **Error handling:** The mutex lock/unlock operations are the critical correctness path. If locking fails, the thread exits early with `thread_complete_success` still set to `false`. This preserves the invariant that success is only true if the lock-hold-unlock sequence completed.
- **Success flag placement:** The flag is set to `false` immediately to ensure a predictable default in error cases. It is only set to `true` after a successful unlock, guaranteeing the mutex was both acquired and released cleanly.
- **Architecture decision:** The thread returns the same pointer it was given, enabling the caller to `pthread_join`, inspect `thread_complete_success`, and free the memory, matching the expected ownership contract.

## Pseudocode Block 3: `start_thread_obtaining_mutex` (threading.c)

```c
/*
 * PSEUDOCODE:
 *   Allocate thread_data on the heap to survive past this call.
 *   Populate the wait timings and mutex pointer.
 *   Set thread_complete_success false until the thread finishes.
 *   Create the thread with threadfunc as the entry point.
 *   Free memory and return false if creation fails.
 *   Return true on success without waiting for completion.
 */
```

### Rationale and methodology
- **Dynamic allocation:** The assignment requires heap allocation so the thread can outlive the creating function. Stack-allocating would be unsafe because the pointer would become invalid as soon as the function returns.
- **Ownership and lifetime:** The pseudocode makes it clear that ownership is transferred to the thread and then back to the joiner via the return pointer. This is why we only free memory in the error path (before the thread is created).
- **Return value contract:** The function returns immediately after `pthread_create`, honoring the requirement that it “should not block for the thread to complete.” This preserves concurrency and allows multiple threads to be created.
- **Error handling:** If `pthread_create` fails, it’s important to free the allocation to avoid leaks. This is explicitly highlighted to encourage disciplined resource management.
- **Architecture decision:** The function only sets `thread_complete_success` to `false` up front. The thread function is responsible for flipping it to `true` once work is done, keeping the success criteria centralized in the thread’s execution path.

## Additional notes for junior developers
- **Why `pthread_create` uses a `void*` parameter:** This is part of the POSIX API design and allows generic usage; the caller can pass a struct pointer to represent multiple arguments. See `man pthread_create` for details: https://man7.org/linux/man-pages/man3/pthread_create.3.html.
- **Why `pthread_mutex_t*` is stored instead of copied:** Mutexes should be shared between threads. Storing a pointer maintains the shared identity; copying a mutex would create a distinct lock and break synchronization.
- **Why error logging is used:** The provided `ERROR_LOG` macro is used to align with the existing file’s logging pattern while keeping stdout/stderr behavior consistent with the rest of the example codebase.
