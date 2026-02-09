# Assignment 5 Part 1 Implementation Plan

## Objective
Implement a socket-based server application named `aesdsocket` with optional daemon mode, persistent packet logging behavior, and startup/shutdown automation.

## Planned Work Breakdown

1. **Scaffold server deliverables**
   - Create `server/` directory.
   - Add source file `server/aesdsocket.c`.
   - Add `server/Makefile` with `default` and `all` targets and `CROSS_COMPILE` compatibility.
   - Add `server/aesdsocket-start-stop` init-style helper using `start-stop-daemon`.

2. **Implement process lifecycle and signal behavior**
   - Parse optional `-d` argument.
   - Initialize syslog usage.
   - Register handlers for `SIGINT` and `SIGTERM`.
   - Implement clean shutdown path that logs exit message, closes sockets, and deletes `/var/tmp/aesdsocketdata`.

3. **Implement socket bring-up and daemonization**
   - Create TCP stream socket.
   - Configure `SO_REUSEADDR`.
   - Bind to `0.0.0.0:9000` and listen.
   - If `-d` is passed, fork after bind/listen succeeds, detach session, and redirect stdio.

4. **Implement client handling and packet processing**
   - Accept client connections in a loop.
   - Log accepted/closed client IP addresses to syslog.
   - Receive stream data, buffer until newline, and append completed packets to `/var/tmp/aesdsocketdata`.
   - Handle dynamic buffer allocation safely.

5. **Implement response behavior**
   - After each newline-terminated packet is appended, send the full file contents back to the connected client.
   - Ensure file response is chunked reads to avoid RAM-size assumptions.

6. **Validate and document**
   - Compile using `make` in `server/`.
   - Update `README.md` with Assignment 5 summary, architecture notes, and commit/date history.
   - Add `code-review-assignment5-part1.md` documenting pseudocode blocks and rationale.
