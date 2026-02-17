# Plan: Assignment 6 Part 1 — Multi-Threaded aesdsocket

## Overview

Extend the Assignment 5 single-threaded aesdsocket to support multiple simultaneous
client connections, each handled by a dedicated pthread. A separate timer thread
appends RFC2822 timestamps every 10 seconds. All file access is serialized via a
pthread mutex.

## Architecture

```
main thread
  starts timer_thread
  accept() loop:
    on new connection: malloc thread_entry, SLIST_INSERT_HEAD, pthread_create
    reap completed threads (join + free) after each accept
    on signal: close(server_fd) -> accept() fails -> exit loop -> join all

connection_thread (one per client):
  recv loop: accumulate data until newline
  on newline: lock mutex -> append to DATA_FILE -> send file -> unlock
  on close/error: free buf, close fd, set entry->complete = true

timer_thread:
  clock_nanosleep(10s) loop
  on wake: format RFC2822 timestamp
  lock mutex -> append timestamp to DATA_FILE -> unlock
```

## Thread Management (SLIST)

Struct:
```c
struct thread_entry {
    pthread_t tid;          // thread ID for pthread_join
    int client_fd;          // file descriptor for this client
    volatile bool complete; // set true by thread on exit
    SLIST_ENTRY(thread_entry) entries;
};
SLIST_HEAD(thread_list, thread_entry) thread_head;
```

On new connection:
```
entry = malloc(thread_entry)
entry->client_fd = client_fd
entry->complete = false
SLIST_INSERT_HEAD(&thread_head, entry)
pthread_create(&entry->tid, NULL, connection_thread, entry)
```

Reaping after each accept (non-blocking, only completed threads):
```
for each entry in thread_head:
    if entry->complete:
        pthread_join(entry->tid)
        SLIST_REMOVE(entry)
        free(entry)
```

## Mutex Strategy

A single pthread_mutex_t data_mutex protects all I/O on /var/tmp/aesdsocketdata.
The critical section spans file open -> write -> close, ensuring no interleaving
between simultaneous connections or the timer thread.

Connection thread on newline:
```
pthread_mutex_lock(&data_mutex)
  fopen(DATA_FILE, "a")
  fwrite(packet)
  fclose()
  send_file_contents(client_fd)   // full file read also under lock
pthread_mutex_unlock(&data_mutex)
```

## Timer Thread

```
while (!caught_signal):
    clock_nanosleep(CLOCK_MONOTONIC, 10s)
    if caught_signal: break
    localtime_r(&now, &tm_info)
    strftime(buf, "timestamp:%a, %d %b %Y %H:%M:%S %z\n", &tm_info)
    pthread_mutex_lock(&data_mutex)
      fopen(DATA_FILE, "a")
      fputs(buf)
      fclose()
    pthread_mutex_unlock(&data_mutex)
```

## Shutdown Sequence

1. Signal handler sets caught_signal and calls close(server_fd)
2. accept() in main loop returns with error -> caught_signal check -> break
3. Main thread calls pthread_join(timer_tid) -- timer sees caught_signal, exits
4. Main thread iterates SLIST, calls pthread_join on all remaining threads
5. cleanup() removes DATA_FILE, closes syslog

## Validation

- make compiles with -Wall -Werror -pthread
- sockettest.sh from assignment-autotest/test/assignment6/
- full-test.sh from repo root
