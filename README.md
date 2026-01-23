# README - ECEN 5713 Assignment 1
The purpose of this assignment is to familiarize students with scripting using Bash. The assignment walks students through preparing their Linux environment for use in this class, as well as the creation of the following function and shell scripts:
- *student-test/assignment1/Test_validate_username.c*
	- **Purpose**: Confirm that the username contained in *conf/username.txt* matches the username found in *examples/autotest-validate.c*
	- **Arguments**: None
	- **Return Values**:
		- No returned values (void)
		- Runs a Unity assertion and will print an assert response to confirm whether the two strings match or not
- *finder-app/finder.sh*
	- **Purpose**: Prints the number of files in a directory and all subdirectories, as well as the number of lines that match a specified string.
	- **Arguments**:
		- *filesdir*: Path to the target directory on the filesystem
		- *searchstr*: String that should be searched for inside of this directory
	- **Return Values**:
		- 1: error. Printed if:
			- Any of the above parameters are not specified
			- *filesdir* does not represent a directory on the filesystem
		- Prints "The number of files are X and the number of matching lines are Y" if successful
- *finder-app/writer.sh*
	- **Purpose**: Creates a new file with name and path *writefile* and content *writestr*, overwriting any existing file and creating the path if it does not exist.
	- **Arguments**:
		- *writefile*: The path and name of the file to be written to
		- *writestr*: The content to be written to that file
	- **Return Values**:
		- 1: error. Printed if the file could not be created.
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
│   ├── finder-test.sh                   (supplied by professor)
│   ├── finder.sh                        (created by jsnapoli1)
│   └── writer.sh                        (created by jsnapoli1)
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

Claude Code was utilized for this project in conjuction with the course's AI policy. Each individual file has a link to a Github Gist that shows how Claude was used.
Here is the final gist, which shows a codebase review utilizing the rubric on Canvas: https://gist.github.com/jsnapoli1/9f229178c296b1b099e7f8e213174ddd

### Prompts Used
- "Please review the documented notes within finder.sh and recommend a preliminary implementation of the features discussed"
- "Please review the documented notes within writer.sh and recommend a preliminary implementation of the features discussed"
- Please run finder-test.sh and full-test.sh to ensure that all functions work properly
### Methodology
I use Claude Code extensively at work, and given the discussed AI policy of the class, I figured I would explain a bit of my methodology for how I plan to use Claude Code within this class.
I subscribe to a philosophy passed down by my mentor, Cliff Brake, called Doc Driven Development. The ideology behind doc-driven-development is to focus on the system first. Claude has gotten extremely good at writing efficient code, however it fails when it comes to the big picture. It often makes changes without considering the effects this will have downstream.
To remedy this, we utilize an approach that focuses on the human side first. It starts in markdown, where the software engineer will write extensively about the necessary changes that Claude needs to make. This includes what functions are needed, what their inputs should be, what their error handling should look like, and of course that their purpose is. Additional notes are left on process-specific information, like how functions should work together, or how resources should be shared efficiently. These plans are then given to Claude for one doc-driven-development "cycle".
A cycle involves four steps:
1. Claude will enter plan mode and review the uncommitted changes you have made in the form of documentation. In doing so, it will ask questions, provide context, or expand upon the plan file you made, extensively fleshing out the idea you have created.
2. Claude enters implementation mode, where it goes and implements the specific functionalities that you have described in your plan file.
3. Claude implements tests for the code it has just written, utilizing the edge cases and error handling scenarios mentioned in the initial documentation
4. Claude updates its own documentation, including updating the CHANGELOG, README, and CLAUDE markdown files, as well as its original plan with details of how the implementation may have changed
Claude also has the ability to build and run tests at any given time during its endeavors, allowing it to catch bugs early and often.
I then have a personal process I go through to ensure code reliability. Each line is reviewed to confirm that memory-safe functions and methodologies are used. I also review functions as a whole to confirm that the general flow through a function is favorable, including confirmation that processes are nonblocking.
This methodology has provided immense benefit in industry, allowing my team and I to focus on system architecture and develop at a significantly faster rate, while still ensuring the same quality we previously achieved.
## Previous README Content
This repo contains public starter source code, scripts, and documentation for Advanced Embedded Software Development (ECEN-5713) and Advanced Embedded Linux Development assignments University of Colorado, Boulder.
-
### Setting Up Git
Use the instructions at [Setup Git](https://help.github.com/en/articles/set-up-git) to perform initial git setup steps. For AESD you will want to perform these steps inside your Linux host virtual or physical machine, since this is where you will be doing your development work.
### Setting up SSH keys
See instructions in [Setting-up-SSH-Access-To-your-Repo](https://github.com/cu-ecen-aeld/aesd-assignments/wiki/Setting-up-SSH-Access-To-your-Repo) for details.
### Specific Assignment Instructions
Some assignments require further setup to pull in example code or make other changes to your repository before starting.  In this case, see the github classroom assignment start instructions linked from the assignment document for details about how to use this repository.
### Testing
The basis of the automated test implementation for this repository comes from [https://github.com/cu-ecen-aeld/assignment-autotest/](https://github.com/cu-ecen-aeld/assignment-autotest/)

The assignment-autotest directory contains scripts useful for automated testing  Use
```
git submodule update --init --recursive
```
to synchronize after cloning and before starting each assignment, as discussed in the assignment instructions.

As a part of the assignment instructions, you will setup your assignment repo to perform automated testing using github actions.  See [this page](https://github.com/cu-ecen-aeld/aesd-assignments/wiki/Setting-up-Github-Actions) for details.

Note that the unit tests will fail on this repository, since assignments are not yet implemented.  That's your job :) 

## Assignment 2 C Writer Notes
### Architectural Decisions
- The C implementation uses `open()` with `O_WRONLY | O_CREAT | O_TRUNC` and a `0644` file mode to match the shell script's behavior of overwriting any existing file while allowing the caller to control directory creation. This mirrors standard file creation permissions for user-writable files.
- Writes are performed via a small `write_all()` helper that loops until all bytes are written, ensuring partial writes from `write()` are handled correctly instead of assuming a single call will write the entire buffer.
- A `sync()` call was intentionally omitted because system-wide flushes can be unfavorable on large systems with many buffers; a comment in code notes where a per-file `fsync()` would be considered if allowed.
- Syslog is initialized with `LOG_USER`, emits a `LOG_DEBUG` message before writing, and logs all error paths (argument validation, open/write/sync/close failures) at `LOG_ERR` for parity with the shell script's explicit error conditions.
- https://chatgpt.com/s/cd_69738c4c6e148191a26aa9b13cc4f84e
- The `finder-app/Makefile` defines `CROSS_COMPILE ?=` and `CC ?= $(CROSS_COMPILE)gcc` so that setting `CROSS_COMPILE=aarch64-none-linux-gnu-` swaps in the cross compiler while leaving native builds unchanged. `CFLAGS ?= -Wall -Wextra -Werror` enables warning coverage and treats warnings as errors to prevent silent issues in the small utility; the `?=` operator allows callers to override these defaults without editing the file.
- Object files are derived from `SRCS` via `OBJS := $(SRCS:.c=.o)` for straightforward source/object mapping, and `clean` removes both the `writer` binary and any `.o` files via `rm -f` to avoid errors when files are missing.
- This Makefile was added in commit `d949d24` on 2026-01-23.
- https://chatgpt.com/s/cd_6973925f9af0819184fc3ae465f7e567

### Finder Test Runner Notes
- The finder test now calls `make clean` before building to guarantee stale `writer` artifacts are removed and the test always exercises a fresh binary for the current tree state.
- The build step explicitly sets `CROSS_COMPILE=` to force a native compile even if the environment exports a cross prefix; this prevents accidentally using a cross-compiled `writer` that cannot run on the host executing the tests.
- The test invokes `./writer` instead of the legacy `writer.sh` to ensure the C implementation is what gets validated by the assignment tests.
- These finder test updates were implemented in commit `80fa474` on 2026-01-23.
- https://chatgpt.com/s/cd_6973951340208191af369d3a0deb14f1

## Makefile Line-by-Line Analysis
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

### Capture the Cross-Compiler Sysroot/Verbose Output
From the repo root on Ubuntu, run:
```
./assignments/assignment2/cross-compile.sh
```
This appends both stdout and stderr from `aarch64-none-linux-gnu-gcc -print-sysroot -v` into `assignments/assignment2/cross-compile.txt`.
