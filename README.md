# README - ECEN 5713 Assignment 3
The purpose of this assignment is to familiarize students with syscalls and how to utilize syscalls to create child processes from parents, as well as open and write to files using various sys flags.

## Assignment 3 - Linux Syscalls

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
