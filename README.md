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
