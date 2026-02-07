# Assignment 4 Part 1 Plan

## Goals
- Implement the `thread_data` struct fields required by the threading assignment.
- Implement `threadfunc` to sleep, lock, hold, unlock, and report success.
- Implement `start_thread_obtaining_mutex` to allocate and launch threads without blocking.
- Document the approach and rationale with pseudocode blocks and a code-review write-up.
- Update README with a summary of work (including commit hashes and dates).

## Execution Plan
1. **Review requirements and tests**
   - Read `examples/threading/threading.c`/`.h` TODOs to identify required behaviors.
   - Note test expectations from `assignment-autotest/test/assignment4/Test_threading.c`.

2. **Define the thread data contract**
   - Add fields to `struct thread_data` for wait times, mutex pointer, and completion state.
   - Add inline pseudocode to explain the data layout.

3. **Implement `threadfunc`**
   - Pseudocode block: cast thread args, sleep to obtain, lock, sleep to release, unlock, set success.
   - Implement the corresponding code below the pseudocode, with error logging and success flag updates.

4. **Implement `start_thread_obtaining_mutex`**
   - Pseudocode block: allocate thread data, populate fields, create thread, return success/failure.
   - Implement the corresponding code below the pseudocode, freeing memory on failure.

5. **Document and summarize**
   - Add a `code-review-assignment4-part1.md` file that includes each pseudocode block and rationale.
   - Update `README.md` with a summary of work including commit hashes and dates.

6. **Validation**
   - Run the relevant tests if feasible, or document why tests were not run.
