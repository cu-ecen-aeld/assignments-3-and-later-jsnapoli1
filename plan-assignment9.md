# Plan - Assignment 9: Custom llseek for aesd-char-driver

## Objective

Add custom seek support to the `aesd-char-driver` by implementing an `llseek` file_operations function that supports all three POSIX positional seek types: `SEEK_SET`, `SEEK_CUR`, and `SEEK_END`.

## Background

The current driver has `read`, `write`, `open`, and `release` in its `file_operations` struct. Without a custom `llseek`, the kernel's default `default_llseek` is used, which operates on `inode->i_size`. Since our driver does not maintain `i_size` (the data lives in a circular buffer, not an inode-backed file), seeking does not work correctly.

We need a custom `llseek` that:
1. Computes the total size of all data in the circular buffer (sum of all entry sizes)
2. Translates the `(offset, whence)` pair into an absolute file position
3. Validates the resulting position is within bounds `[0, total_size]`
4. Updates `filp->f_pos` accordingly

## Implementation Steps

### Step 1: Add a `total_size` helper to the circular buffer

- Add a function `aesd_circular_buffer_total_size()` in `aesd-circular-buffer.c` / `.h`
- This function iterates over all valid entries and sums their `size` fields
- This gives us the "end of file" position needed for `SEEK_END`

### Step 2: Implement `aesd_llseek` in `main.c`

- Acquire the device mutex to protect against concurrent buffer modifications
- Compute `total_size` from the circular buffer
- Use `fixed_size_llseek()` if available, or manually switch on `whence`:
  - `SEEK_SET`: new position = `offset`
  - `SEEK_CUR`: new position = `filp->f_pos + offset`
  - `SEEK_END`: new position = `total_size + offset`
- Validate: if new position < 0 or > total_size, return `-EINVAL`
- Update `filp->f_pos` and return the new position
- Release the mutex

### Step 3: Register `llseek` in `file_operations`

- Add `.llseek = aesd_llseek` to the `aesd_fops` struct

### Step 4: Documentation

- Create `code-review-assignment9.md` with block-by-block rationale
- Update `README.md` with assignment 9 summary, commit hashes, and architectural notes

## Architectural Considerations

- **Mutex protection**: The seek operation reads buffer metadata (entry sizes), so it must hold the device mutex to prevent races with concurrent writes that could add/evict entries mid-calculation
- **No `inode->i_size` tracking**: Rather than maintaining `i_size` on every write (which adds complexity to the write path), we compute total size on-demand during seek. This is O(n) where n=10 entries max, so the cost is negligible
- **Boundary validation**: We allow seeking to exactly `total_size` (one past the last byte) to match standard POSIX semantics where `lseek(fd, 0, SEEK_END)` returns the file size
- **Kernel API choice**: We use `fixed_size_llseek()` from `<linux/fs.h>` if it fits, otherwise implement the three-way switch manually. The manual approach gives us more control and is more instructive

## Commit Strategy

1. Commit the plan document
2. Commit the `total_size` helper function
3. Commit the `aesd_llseek` implementation and `file_operations` registration
4. Commit the code-review document
5. Commit the README update
