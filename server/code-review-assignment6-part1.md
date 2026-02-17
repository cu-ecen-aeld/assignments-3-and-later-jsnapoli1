# Code Review: Assignment 6 Part 1 — Multi-Threaded aesdsocket

## Overview

This document explains the architectural decisions and implementation rationale for
the multi-threaded aesdsocket introduced in Assignment 6 Part 1.

---

## Thread-Per-Connection Model

**Decision:** Spawn one pthread per accepted connection rather than a thread pool or
select/poll-based event loop.

**Rationale:** The assignment explicitly requires spawning a new thread per connection.
The expected connection count is low (test scripts use 3 simultaneous connections),
so the overhead of per-thread stack allocation is acceptable. A thread pool would
require a queue and coordination overhead not warranted here.

**Reference:** POSIX Threads Programming Guide (Lawrence Livermore)

---

## SLIST (Singly-Linked List via sys/queue.h)

**Decision:** Use `SLIST_HEAD`/`SLIST_INSERT_HEAD`/`SLIST_REMOVE` from `<sys/queue.h>`.

**Rationale:** `<sys/queue.h>` is a POSIX-standard macro set for linked lists. It
generates inline code with no dynamic allocation overhead for the list operations
themselves. A singly-linked list is sufficient since we only need forward iteration
(no reverse traversal needed). The assignment references this API specifically.

**Why not std::list or glib?** This is a C assignment; `sys/queue.h` is the idiomatic
C approach and available on all Linux systems.

**SLIST_INSERT_HEAD vs _TAIL:** O(1) insertion at head is sufficient since ordering
of thread cleanup doesn't matter. _TAIL would require tracking the tail pointer.

---

## Blocking accept() in Main Thread

**Decision:** Call `accept()` in the main thread with no timeout, blocking indefinitely.

**Rationale:** The assignment explicitly recommends this approach: "Block on accept in
the main loop and free completed threads ONLY after starting the next thread." This
avoids busy-waiting and allows the OS to schedule other threads efficiently. Under
valgrind, non-blocking accept with a tight loop causes thread starvation because
valgrind's overhead prevents the socket threads from running.

**Shutdown trigger:** Closing `server_fd` in the signal handler causes `accept()` to
return EBADF/EINVAL immediately, unblocking the main thread cleanly without requiring
a `select()`/`poll()` call with a timeout.

---

## pthread_mutex for File Synchronization

**Decision:** Use `pthread_mutex_t data_mutex` to serialize all file I/O.

**Rationale:** File system synchronization (e.g., `fsync`, `O_APPEND`) cannot guarantee
atomicity for the read-modify-write pattern we need (append line, then read entire file
back). A mutex ensures that:
1. Two simultaneous clients cannot interleave their writes into the file
2. The timer thread cannot write a timestamp mid-packet
3. `send_file_contents` reads a consistent snapshot under the same lock

**Scope of lock:** The lock is held for the full sequence: fopen(append) -> fwrite ->
fclose -> fopen(read) -> fread/send -> fclose. This is intentionally broad to prevent
any partial state being visible to other threads.

---

## Dynamic Buffer Growth with realloc

**Decision:** Use `realloc` to grow the per-connection line buffer as data arrives.

**Rationale:** The assignment states packets are shorter than available heap but longer
than stack-suitable sizes are possible. A fixed stack buffer would silently truncate
packets. `realloc` allows the buffer to grow incrementally as data arrives, and is
freed when the packet is complete or the connection closes.

**Error handling:** On `realloc` failure, we log the error, free the buffer, and goto
the cleanup label — avoiding a use-after-free if we had assigned the return value back
to the same pointer.

---

## clock_nanosleep for Timer

**Decision:** Use `clock_nanosleep(CLOCK_MONOTONIC, ...)` in a dedicated thread rather
than `SIGALRM` or `timer_create`.

**Rationale:**
- `SIGALRM` interacts poorly with pthreads (delivered to arbitrary thread, requires
  signal masking coordination).
- `timer_create` with `SIGEV_SIGNAL` has the same issue. `SIGEV_THREAD` creates its
  own thread internally, adding complexity.
- A dedicated thread with `clock_nanosleep` is simple, self-contained, and exits cleanly
  when `caught_signal` is set.

**CLOCK_MONOTONIC vs CLOCK_REALTIME:** MONOTONIC is used for the sleep interval to
avoid drift if the system clock is adjusted (NTP). The wall clock is read separately
via `time(NULL)` + `localtime_r` for the timestamp content.

---

## localtime_r vs localtime

**Decision:** Use `localtime_r` (reentrant) rather than `localtime`.

**Rationale:** `localtime` returns a pointer to a static buffer that is not
thread-safe. `localtime_r` writes to a caller-provided `struct tm`, which is safe
for concurrent use from multiple threads.

---

## RFC2822 Timestamp Format

**Format string:** `"timestamp:%a, %d %b %Y %H:%M:%S %z\n"`

This produces output like: `timestamp:Mon, 16 Feb 2026 19:55:00 +0000`

The `sockettest.sh` validation script filters out lines starting with `timestamp:` 
before comparing file contents. The format must match exactly or the binary-content
check fails (the test diffs output with expected, fails if binary chars appear).

---

## Graceful Shutdown Threading Order

1. `signal_handler` sets `caught_signal = 1`, closes `server_fd`
2. `accept()` returns error, main loop detects `caught_signal`, breaks
3. `pthread_join(timer_tid)` — timer thread checks `caught_signal` at top of loop,
   exits cleanly (the in-progress `clock_nanosleep` may complete its remaining slice)
4. SLIST iteration joins all remaining connection threads — any thread still in
   `recv()` will return 0/error when the client closes, or when we close the fd
5. `pthread_mutex_destroy` — safe now that all threads are joined
6. `cleanup()` — removes `/var/tmp/aesdsocketdata`, closes syslog

**Why join timer before connections?** Timer is the simplest to join (no client state)
and will exit immediately if caught_signal is set. Connection threads may take
slightly longer to drain in-flight data.

---

## Commit History

See `git log --oneline` for per-function commit breakdown.
