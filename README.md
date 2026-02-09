# README - ECEN 5713 Assignments 3 and 4
This repository includes prior Assignment 3 work and Assignment 4 Part 1 threading updates. Assignment 4 focuses on POSIX thread synchronization, dynamic thread argument ownership, and completion-status reporting for joinable worker threads.

## Assignment 3 Part 1 - Linux Syscalls

### Procedure:
- Implement modifications to examples/systemcalls/systemcalls.c
	- Find labeled TODO's
		- In do_system(), implement the system() call wrapper
			- Call system() with provided cmd parameter
			- Return true if command is completed successfully
			- Return false if there was a failure
		- In do_exec(), execute a command using fork/execv/wait
			- Use fork() to create a child process
			- Use execv() to execute the command
			- Use command[0] as the full path to the executable
			- Use the remaining args as the second arg to execv()
			- Wait for the child process to complete
			- Return success/failure based on the command's exit status
		- In do_exec_redirect()
			- Same changes as do_exec but no stdout redirect
	- See the provided test code in test/assignment3/Test_systemcalls.c
	- Place fflush(stdout) before the call to fork() to avoid duplicate prints
- Run ./unit-test.sh to test implementation

## Assignment 3 Part 2 - Linux Kernel and Root Filesystem Build

### Procedure:
- Implement modifications to finder-app/manual-linux.sh
	- Complete all TODO sections in the script
	- Build the Linux kernel for ARM64 architecture
		- Clone Linux kernel repository (v5.15.163)
		- Configure with defconfig and cross-compile
	- Create the root filesystem staging directory
		- Create base directories (bin, dev, etc, home, lib, lib64, proc, sbin, sys, tmp, usr, var)
	- Build and install BusyBox
		- Clone BusyBox repository
		- Configure with defconfig and cross-compile
		- Install to rootfs
	- Copy required shared libraries from the cross-compiler sysroot
		- ld-linux-aarch64.so.1, libc.so.6, libm.so.6, libresolv.so.2
	- Cross-compile the writer utility and copy finder app files to rootfs
	- Create initramfs.cpio.gz for QEMU boot
- Updated .github/workflows/github-actions.yml to add full-test job
- Run ./full-test.sh to test implementation

## Assignment 4 Part 1 - Threading

### Procedure:
- Implement modifications to `examples/threading/threading.c` and `examples/threading/threading.h`
  - Find labeled TODOs
    - In `struct thread_data`, add required thread configuration fields
      - `wait_to_obtain_ms`
      - `wait_to_release_ms`
      - mutex used by the thread
      - `thread_complete_success` status flag
    - In `threadfunc`
      - Cast the input argument using `struct thread_data* thread_func_args = (struct thread_data *) thread_param;`
      - Sleep for `wait_to_obtain_ms` milliseconds
      - Lock the provided mutex
      - Sleep for `wait_to_release_ms` milliseconds while holding the mutex
      - Unlock the mutex
      - Set `thread_complete_success` true only on successful completion
      - Return the thread-data pointer for the joiner to inspect/free
    - In `start_thread_obtaining_mutex`
      - Dynamically allocate `thread_data` on the heap
      - Populate wait times, mutex pointer, and default completion status
      - Start a thread with `pthread_create` using `threadfunc`
      - Return `true` when thread startup succeeds and `false` on failure
      - Do not block for completion (join should happen elsewhere)
- Added `plan-assignment4-part1.md` for implementation planning and sequencing
- Added `code-review-assignment4-part1.md` documenting pseudocode blocks, rationale, and architectural decisions

### Work Summary (with commit hashes and dates)
- 059de7a (2026-01-31): Added `thread_data` fields and implemented `threadfunc` sleep/lock/unlock flow with completion reporting
- d8a4f14 (2026-01-31): Implemented `start_thread_obtaining_mutex` with dynamic allocation and `pthread_create` error handling
- d54968b (2026-01-31): Added Assignment 4 plan and code-review documentation
- 41c512a (2026-01-31): Updated README with Assignment 4 summary and architecture notes

### Architectural Notes
- `thread_data` serves as the ownership and communication contract between creator and worker thread; the worker returns this pointer on exit for join-time cleanup
- `start_thread_obtaining_mutex` is intentionally non-blocking to allow concurrent thread creation and scheduling
- `thread_complete_success` is initialized false and only promoted to true after successful lock/sleep/unlock sequence

## Repository Structure
```
assignment-1-jsnapoli1/
├── .github/
│   └── workflows/
│       └── github-actions.yml            (supplied by professor, modified for assignment 3)
├── .gitignore                            (supplied by professor)
├── .gitlab-ci.yml                        (supplied by professor)
├── .gitmodules                           (supplied by professor)
├── CMakeLists.txt                        (supplied by professor)
├── LICENSE                               (supplied by professor)
├── README.md                             (supplied by professor)
├── full-test.sh                          (supplied by professor)
├── unit-test.sh                          (supplied by professor)
├── assignments/
│   └── assignment2/
│       └── cross-compile.sh             (created for assignment 2)
├── conf/
│   ├── assignment.txt                   (supplied by professor)
│   └── username.txt                     (supplied by professor)
├── examples/
│   ├── autotest-validate/
│   │   ├── .gitignore                   (supplied by professor)
│   │   ├── Makefile                     (supplied by professor)
│   │   ├── autotest-validate-main.c     (supplied by professor)
│   │   ├── autotest-validate.c          (supplied by professor)
│   │   └── autotest-validate.h          (supplied by professor)
│   └── systemcalls/
│       ├── systemcalls.c                (modified for assignment 3)
│       └── systemcalls.h                (supplied by professor)
├── finder-app/
│   ├── Makefile                         (created for assignment 2)
│   ├── finder-test.sh                   (supplied by professor, modified for assignment 2)
│   ├── finder.sh                        (created for assignment 1)
│   ├── manual-linux.sh                  (supplied by professor, modified for assignment 3)
│   ├── writer.c                         (created for assignment 2)
│   └── writer.sh                        (created for assignment 1)
├── student-test/
│   └── assignment1/
│       ├── Test_validate_username.c
│       │                                   (supplied by professor)
│       └── Test_validate_username_Runner.c
│                                           (supplied by professor)
├── assignment-autotest/                 (submodule – supplied by professor)
│   ├── CMakeLists.txt
│   ├── auto_generate.sh
│   ├── full-test.sh
│   ├── test-basedir.sh
│   ├── test-unit.sh
│   ├── test/
│   │   ├── assignment1/
│   │   ├── assignment2/
│   │   └── ...
│   ├── Unity/                           (submodule – third party)
│   └── ...
└── build/                               (generated build artifacts)
```

(Disclosure: The above file tree was generated as markdown text by Claude Code, to simplify formatting)

## AI Use

ChatGPT Codex was used to aid in this assignment. All chats, along with Codex's implementation notes can be found below:

### Assignment 3 Implementation Notes

#### Systemcalls Implementation Notes
- The `do_system` implementation checks `system()`'s status with `WIFEXITED`/`WEXITSTATUS` to treat only a clean `0` exit as success and to flag errors when the shell failed to launch or the command returned non-zero; this mirrors the tests’ success/failure expectations without guessing at shell-specific status conventions.
- Both `do_exec` and `do_exec_redirect` call `fflush(stdout)` before `fork()` to avoid duplicate buffered output from the parent and child processes when stdout is line-buffered or fully buffered, which can otherwise cause repeated prints.
- The child process uses `_exit(EXIT_FAILURE)` if `execv()` fails so it does not re-run parent atexit handlers or flush shared stdio buffers, keeping the failure path deterministic.
- `do_exec_redirect` opens the output file with `O_WRONLY | O_TRUNC | O_CREAT`, duplicates it onto `STDOUT_FILENO` via `dup2()`, and closes the descriptor in both parent and child so the output redirection mirrors the reference fork/dup2 example (https://stackoverflow.com/a/13784315/1446624).
- https://chatgpt.com/s/cd_69780eadad308191b776afa22055ff5e

#### Manual Linux Script Implementation Notes
- Updated kernel repository URL from `linux-stable.git` to `linux.git` and version from v5.1.10 to v5.15.163 per assignment requirements.
- Added `realpath -m` for OUTDIR to handle relative paths and non-existent directories properly.
- Kernel build uses `defconfig` for ARM64 architecture with cross-compilation, building in parallel with `-j$(nproc)`.
- Root filesystem directories are created following standard Linux FHS layout (bin, dev, etc, home, lib, lib64, proc, sbin, sys, tmp, usr/bin, usr/lib, usr/sbin, var, home/conf).
- BusyBox is configured with `make distclean && make defconfig`, then cross-compiled and installed to rootfs using `CONFIG_PREFIX`.
- Shared libraries are copied from the cross-compiler sysroot (obtained via `${CROSS_COMPILE}gcc -print-sysroot`) to support dynamic linking on the target.
- The finder-test.sh script path is modified with `sed` to use `conf/assignment.txt` instead of `../conf/assignment.txt` since the rootfs layout differs from the source tree.
- The initramfs is created using `find | cpio -H newc -ov --owner root:root` and compressed with gzip for QEMU boot.
- https://chatgpt.com/s/cd_697a39c38894819197b1dc859f75dca8 

## Assignment 5 Part 1 - Socket Server (`aesdsocket`)

### Procedure:
- Added `server/aesdsocket.c` implementing a TCP server on port `9000`
  - Supports foreground mode and daemon mode via `-d`
  - Handles `SIGINT`/`SIGTERM` for graceful shutdown and cleanup
  - Logs accepted and closed connections using syslog with client IP addresses
  - Appends newline-terminated packets to `/var/tmp/aesdsocketdata`
  - Sends full file contents back to client after each complete packet
  - Removes `/var/tmp/aesdsocketdata` at graceful exit
- Added `server/Makefile`
  - Supports `default` and `all` targets
  - Supports cross-compilation with `CROSS_COMPILE`
- Added `server/aesdsocket-start-stop`
  - Uses `start-stop-daemon` to start `aesdsocket` with `-d`
  - Uses `SIGTERM` on stop for graceful cleanup
- Added `plan-assignment5-part1.md` implementation plan
- Added `code-review-assignment5-part1.md` with pseudocode block rationale and architectural review

### Work Summary (with commit hashes and dates)
- 1e0d1e6 (2026-02-09): Implemented Assignment 5 Part 1 server deliverables (`aesdsocket`, Makefile, startup script, planning and code-review docs)

### Architectural Notes
- Connection handling is intentionally synchronous and single-client-at-a-time to prioritize deterministic correctness for assignment packet semantics.
- Packet completion is defined by newline detection in a stream-oriented TCP channel, requiring explicit buffering and carryover across `recv()` boundaries.
- File readback is chunked to avoid assumptions about in-memory file sizing.
