# Assignment 5 Part 1 Code Review

This review documents the pseudocode blocks implemented in `server/aesdsocket.c` and explains the methodology, architectural choices, and non-obvious syntax.

## Pseudocode Block 1: Packet Assembly and Request/Response Flow

```c
// Pseudocode block: Receive byte stream, build newline-terminated packets, and process each packet.
// 1) Receive bytes in a loop until client disconnects or fatal error occurs.
// 2) Grow a heap-backed packet buffer and append incoming bytes.
// 3) Every time a newline is found, append that full packet to the data file.
// 4) After each completed packet append, transmit the full data file back to the client.
// 5) Keep any trailing partial packet bytes for the next recv() call.
```

### Why this approach
- TCP is stream-oriented, so message boundaries are not preserved by `recv()`. The implementation therefore assembles logical packets in user space, splitting only on `\n`.
- `realloc()` is used to grow a packet buffer to support variable-length packets while preserving previously read bytes.
- Each detected newline triggers file append + full-file sendback to satisfy assignment semantics exactly per packet completion.
- `memmove()` shifts leftover partial bytes to the front after newline processing. This is required because source/destination ranges may overlap.

### Notable syntax and decisions
- `recv_buffer[received] = '\0'` is intentionally included so string functions like `strchr` can safely inspect current buffered data paths.
- Allocation failure path logs to syslog and discards the in-flight packet. This is a deliberate resilience tradeoff requested by assignment text.
- Sendback uses looped `send()` calls to handle partial writes, which is expected behavior on sockets.

### Architectural impact
- This keeps protocol behavior deterministic and line-based while allowing arbitrary chunking at transport layer boundaries.
- Processing is synchronous per client connection, which simplifies correctness and fits assignment requirements.

## Pseudocode Block 2: Daemonization Sequence

```c
// Pseudocode block: Daemonize after bind() succeeded.
// 1) fork() and let parent exit so child can continue detached.
// 2) Create a new session with setsid().
// 3) Move to root directory and reset umask.
// 4) Redirect stdin/stdout/stderr to /dev/null.
```

### Why this approach
- Assignment requires daemon mode to fork only after bind readiness is known; this avoids daemonizing a process that cannot open its service port.
- `setsid()` creates a new session so the process no longer has a controlling terminal.
- `chdir("/")` and `umask(0)` are standard daemon practices to avoid locking mount points and to use explicit permissions.
- Redirecting stdio to `/dev/null` prevents accidental terminal I/O from detached process context.

### Notable syntax and decisions
- Parent exits using `exit(EXIT_SUCCESS)` immediately after successful `fork()`, leaving child active.
- `dup2()` is used to atomically map `/dev/null` FD onto `STDIN_FILENO`, `STDOUT_FILENO`, and `STDERR_FILENO`.

### Architectural impact
- Cleanly separates interactive foreground mode from production daemon mode without duplicating server logic.

## Pseudocode Block 3: Server Socket Bring-up and Main Accept Loop

```c
// Pseudocode block: Create/bind/listen server socket and then serve clients until shutdown signal.
// 1) Create AF_INET stream socket.
// 2) Set SO_REUSEADDR and bind to port 9000 on INADDR_ANY.
// 3) Start listening for clients.
// 4) Optionally daemonize only after successful bind/listen steps.
// 5) accept() clients in a loop, logging connect/disconnect events and handling each client stream.
```

### Why this approach
- Explicit stepwise socket setup keeps failure points isolated and ensures `-1` return behavior if any networking stage fails.
- `SO_REUSEADDR` mitigates restart issues when sockets are in `TIME_WAIT`.
- `accept()` loop continues until `SIGINT`/`SIGTERM` flag is observed, enabling long-running service behavior.
- IP logging via `inet_ntop()` provides human-readable address strings for syslog requirements.

### Notable syntax and decisions
- Signal handler sets only a `sig_atomic_t` flag (`exit_requested`) to remain async-signal-safe.
- `accept()` error handling treats `EINTR` specially so signal interruptions do not incorrectly abort normal operation.
- Cleanup unlinks `/var/tmp/aesdsocketdata` on exit, matching shutdown requirements.

### Architectural impact
- Separates lifecycle concerns (startup, runtime, shutdown) into predictable phases.
- Ensures graceful termination and resource cleanup without adding unnecessary threading complexity.

## External References Used
- Linux `man` pages for: `socket(2)`, `bind(2)`, `listen(2)`, `accept(2)`, `recv(2)`, `send(2)`, `sigaction(2)`, `daemon(7)`, `syslog(3)`.
- Beej's Guide to Network Programming (for TCP partial read/write handling patterns).
