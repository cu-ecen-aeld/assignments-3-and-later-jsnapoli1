# README - ECEN 5713 Assignment 2
The purpose of this assignment is to familiarize students with cross compiling applications for ARM processors when developing on an x86 machine. The assignment prompts the user to convert one of the scripts from Assignment 1 to a .c file, compile it, and run tests. A Makefile was set up to allow users to compile for either the native target or a cross-compiled architecture.
## Assignment 2 Build + Cross-Compile Capture (Ubuntu)
### Build with the Makefile
From the `finder-app/` directory, run a native build using the default toolchain:
```
cd finder-app
make
```
To cross-compile, pass the toolchain prefix via `CROSS_COMPILE`:
```
cd finder-app
make clean
make CROSS_COMPILE=aarch64-none-linux-gnu-
```
### Procedure:
- Adapt the writer.sh script from assignment 1 to writer.c, utilizing file IO rather than shell calls.
	- **Purpose**: Create a new file with name and path *file* and content *string*.
	- **Arguments**:
		- file: the path and name of the file
		- string: the content to be written to the file
	- **Return Values**:
		- 1: error. Printed if the file could not be created
	- The .c file does not need to create a directory if that directory does not exist
	- Setup syslog logging for the tool with the LOG_USER call
	- Using syslog, write a message that says "Writing < string > to < file >", where string is the text written, and file is the file created by the script. This should be at the LOG_DEBUG level.
	- Use syslog to log any unexpected errors at the LOG_ERR level
- Create a make file including the following:
	- Default target to build the writer binary
	- Clean target that removes the writer binary and all .o files
	- Support for cross compilation with the CROSS_COMPILE flag
		- When user specifies CROSS_COMPILE aarch64-none-linux-gnu-
- Update finder-test.sh with the following:
	- Clean previous build artifacts
	- Compile the writer for the native architecture
	- Use the writer binary that was compiled rather than the writer.sh script
- Create a script that appends the arm toolchain call with the -print-sysroot and -v options, and save this output to a text file.
	- This file is `assignments/assignment2/cross-compile.txt`
	- It is created by `assignments/assignment2/cross-compile.sh`
## Repository Structure
```
assignment-1-jsnapoli1/
├── .github/
│   └── workflows/
│       └── github-actions.yml            (supplied by professor)
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
│   └── autotest-validate/
│       ├── .gitignore                   (supplied by professor)
│       ├── Makefile                     (supplied by professor)
│       ├── autotest-validate-main.c     (supplied by professor)
│       ├── autotest-validate.c          (supplied by professor)
│       └── autotest-validate.h          (supplied by professor)
├── finder-app/
│   ├── Makefile                         (created for assignment 2)
│   ├── finder-test.sh                   (supplied by professor, modified for assignment 2)
│   ├── finder.sh                        (created for assignment 1)
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
### Assignment 2 C Writer Notes
#### writer.c Implementation Notes
- The C implementation uses `open()` with `O_WRONLY | O_CREAT | O_TRUNC` and a `0644` file mode to match the shell script's behavior of overwriting any existing file while allowing the caller to control directory creation. This mirrors standard file creation permissions for user-writable files.
- Writes are performed via a small `write_all()` helper that loops until all bytes are written, ensuring partial writes from `write()` are handled correctly instead of assuming a single call will write the entire buffer.
- A `sync()` call was intentionally omitted because system-wide flushes can be unfavorable on large systems with many buffers; a comment in code notes where a per-file `fsync()` would be considered if allowed.
- Syslog is initialized with `LOG_USER`, emits a `LOG_DEBUG` message before writing, and logs all error paths (argument validation, open/write/sync/close failures) at `LOG_ERR` for parity with the shell script's explicit error conditions.
- https://chatgpt.com/s/cd_69738c4c6e148191a26aa9b13cc4f84e

#### Makefile Implementation Notes
- The `finder-app/Makefile` defines `CROSS_COMPILE ?=` and `CC ?= $(CROSS_COMPILE)gcc` so that setting `CROSS_COMPILE=aarch64-none-linux-gnu-` swaps in the cross compiler while leaving native builds unchanged. `CFLAGS ?= -Wall -Wextra -Werror` enables warning coverage and treats warnings as errors to prevent silent issues in the small utility; the `?=` operator allows callers to override these defaults without editing the file.
- Object files are derived from `SRCS` via `OBJS := $(SRCS:.c=.o)` for straightforward source/object mapping, and `clean` removes both the `writer` binary and any `.o` files via `rm -f` to avoid errors when files are missing.

The following analysis walks through each line of `finder-app/Makefile` and ties it back to the assignment requirements and behavior.

1. `CROSS_COMPILE ?=` defines an optional toolchain prefix and defaults to empty so native builds use the host toolchain when no cross prefix is supplied.
2. `CC ?= $(CROSS_COMPILE)gcc` selects the compiler based on the prefix; for example, `CROSS_COMPILE=aarch64-none-linux-gnu-` yields `aarch64-none-linux-gnu-gcc`, while an empty prefix yields `gcc`.
3. `CFLAGS ?= -Wall -Wextra -Werror` sets strict warning flags as the default while still allowing overrides via the command line or environment.
4. `TARGET := writer` names the output binary for the build.
5. `SRCS := writer.c` declares the source file list for the target.
6. `OBJS := $(SRCS:.c=.o)` maps source files to their corresponding object files for compilation and linking.
7. `.PHONY: all clean` marks `all` and `clean` as phony targets so they always run regardless of file names.
8. `all: $(TARGET)` sets the default build target so running `make` builds the writer application.
9. `$(TARGET): $(OBJS)` declares that the final binary depends on the compiled object files.
10. `$(CC) $(CFLAGS) -o $@ $^` links the binary using the selected compiler and flags; `$@` expands to `writer` and `$^` expands to all object prerequisites.
11. `%.o: %.c` defines a generic rule for compiling any C source into an object file.
12. `$(CC) $(CFLAGS) -c $< -o $@` compiles a single source file into an object file; `$<` is the source and `$@` is the output object.
13. `clean:` introduces the cleanup target required by the assignment.
14. `rm -f $(TARGET) $(OBJS)` removes the writer binary and all object files, using `-f` to avoid errors when files are missing.

- This Makefile was added in commit `d949d24` on 2026-01-23.
- https://chatgpt.com/s/cd_6973925f9af0819184fc3ae465f7e567

#### Finder Test Implementation Notes
- The finder test now calls `make clean` before building to guarantee stale `writer` artifacts are removed and the test always exercises a fresh binary for the current tree state.
- The build step explicitly sets `CROSS_COMPILE=` to force a native compile even if the environment exports a cross prefix; this prevents accidentally using a cross-compiled `writer` that cannot run on the host executing the tests.
- The test invokes `./writer` instead of the legacy `writer.sh` to ensure the C implementation is what gets validated by the assignment tests.
- These finder test updates were implemented in commit `80fa474` on 2026-01-23.
- https://chatgpt.com/s/cd_6973951340208191af369d3a0deb14f1

#### Capture the Cross-Compiler Sysroot/Verbose Output
From the repo root on Ubuntu, run:
```
./assignments/assignment2/cross-compile.sh
```
This appends both stdout and stderr from `aarch64-none-linux-gnu-gcc -print-sysroot -v` into `assignments/assignment2/cross-compile.txt`.

#### Systemcalls Implementation Notes
- The `do_system` implementation checks `system()`'s status with `WIFEXITED`/`WEXITSTATUS` to treat only a clean `0` exit as success and to flag errors when the shell failed to launch or the command returned non-zero; this mirrors the tests’ success/failure expectations without guessing at shell-specific status conventions.
- Both `do_exec` and `do_exec_redirect` call `fflush(stdout)` before `fork()` to avoid duplicate buffered output from the parent and child processes when stdout is line-buffered or fully buffered, which can otherwise cause repeated prints.
- The child process uses `_exit(EXIT_FAILURE)` if `execv()` fails so it does not re-run parent atexit handlers or flush shared stdio buffers, keeping the failure path deterministic.
- `do_exec_redirect` ignores `outputfile` because the updated assignment requirement requests the same fork/execv/wait flow as `do_exec` without stdout redirection; the `(void)outputfile` cast documents the intentional unused parameter.
- These systemcalls updates were implemented in commit `dda51894f81506d5962c9ca1ef79cbf802c6f6e2` on 2026-01-27.
