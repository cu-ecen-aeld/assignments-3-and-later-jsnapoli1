# AESD Assignment 8 — Implementation Plan

## 1. File Tree

### Files to modify:
```
aesd-char-driver/aesdchar.h          — add circular buffer, mutex, partial-write fields to aesd_dev
aesd-char-driver/main.c              — implement open, read, write, init, cleanup
server/aesdsocket.c                  — add USE_AESD_CHAR_DEVICE compile-time switch
server/Makefile                      — pass -DUSE_AESD_CHAR_DEVICE=1
```

### Files already present (no changes needed):
```
aesd-char-driver/aesd-circular-buffer.c   — circular buffer implementation (from assignment 7)
aesd-char-driver/aesd-circular-buffer.h   — circular buffer header
aesd-char-driver/Makefile                 — already builds main.o + aesd-circular-buffer.o into aesdchar.ko
aesd-char-driver/aesdchar_load            — creates /dev/aesdchar after insmod
aesd-char-driver/aesdchar_unload          — rmmod + removes /dev/aesdchar
```

## 2. Pseudocode

### 2.1 `aesdchar.h` — Device struct additions

```c
struct aesd_dev {
    struct aesd_circular_buffer buffer;   // circular buffer holding up to 10 entries
    struct mutex lock;                    // protects buffer + partial write state
    char *partial_buf;                    // accumulates bytes until '\n'
    size_t partial_len;                   // current length of partial_buf
    struct cdev cdev;                     // char device structure (already present)
};
```

### 2.2 `main.c` — Function pseudocode

#### `aesd_open()`
```
1. Get aesd_dev* from inode->i_cdev using container_of
2. Store in filp->private_data for use by read/write
3. Return 0
```

#### `aesd_release()`
```
1. Nothing to do — no per-file state to free
2. Return 0
```

#### `aesd_write(filp, buf, count, f_pos)`
```
1. Lock mutex
2. Reallocate partial_buf to hold (partial_len + count) bytes
3. copy_from_user into partial_buf + partial_len
4. Update partial_len += count
5. Scan partial_buf for '\n'
6. If '\n' found:
   a. Create an aesd_buffer_entry with buffptr=partial_buf, size=partial_len
   b. Check if the circular buffer is full — if so, kfree the entry being evicted
   c. Call aesd_circular_buffer_add_entry()
   d. Set partial_buf = NULL, partial_len = 0
      (ownership transferred to circular buffer)
7. Unlock mutex
8. Return count (all bytes accepted)
```

#### `aesd_read(filp, buf, count, f_pos)`
```
1. Lock mutex
2. Call aesd_circular_buffer_find_entry_offset_for_fpos(buffer, *f_pos, &entry_offset)
3. If NULL returned: unlock, return 0 (EOF)
4. bytes_available = entry->size - entry_offset
5. bytes_to_copy = min(count, bytes_available)
6. copy_to_user(buf, entry->buffptr + entry_offset, bytes_to_copy)
7. *f_pos += bytes_to_copy
8. Unlock mutex
9. Return bytes_to_copy
```

#### `aesd_init_module()`
```
1. alloc_chrdev_region (already present)
2. memset aesd_device (already present)
3. Initialize: aesd_circular_buffer_init(&aesd_device.buffer)
4. Initialize: mutex_init(&aesd_device.lock)
5. Set partial_buf = NULL, partial_len = 0  (done by memset)
6. Call aesd_setup_cdev (already present)
```

#### `aesd_cleanup_module()`
```
1. cdev_del (already present)
2. Iterate circular buffer with AESD_CIRCULAR_BUFFER_FOREACH:
   kfree each non-NULL entry->buffptr
3. kfree(aesd_device.partial_buf) if non-NULL
4. unregister_chrdev_region (already present)
```

### 2.3 `server/aesdsocket.c` — USE_AESD_CHAR_DEVICE switch

```c
#ifndef USE_AESD_CHAR_DEVICE
#define USE_AESD_CHAR_DEVICE 1
#endif

#if USE_AESD_CHAR_DEVICE
#define DATA_FILE "/dev/aesdchar"
#else
#define DATA_FILE "/var/tmp/aesdsocketdata"
#endif
```

When `USE_AESD_CHAR_DEVICE == 1`:
- Do NOT start timer_thread
- Do NOT call remove(DATA_FILE) in cleanup or startup
- In connection_thread: open fd before write, close after write; open fd before read-back, close after read-back
  (do NOT hold fd open across connection lifetime)

When `USE_AESD_CHAR_DEVICE == 0`:
- Keep all existing behavior unchanged

## 3. Locking Strategy

### Kernel driver (`aesd_dev.lock` mutex):
- **Protects**: `aesd_dev.buffer` (circular buffer), `aesd_dev.partial_buf`, `aesd_dev.partial_len`
- **Locked during**: entire `aesd_read()` and `aesd_write()` operations
- **Atomic operation**: A complete write (copy_from_user + scan for '\n' + optional commit to buffer) is a single critical section
- **Rationale**: Multiple processes can have /dev/aesdchar open simultaneously; mutex ensures consistent state

### Socket server (`data_mutex`):
- Unchanged — protects file I/O to DATA_FILE (whether /var/tmp/aesdsocketdata or /dev/aesdchar)

## 4. Build Switch Strategy

### `server/Makefile`:
Add `-DUSE_AESD_CHAR_DEVICE=1` to CFLAGS by default:
```makefile
CFLAGS ?= -Wall -Werror -g -DUSE_AESD_CHAR_DEVICE=1
```

### `server/aesdsocket.c`:
```c
#ifndef USE_AESD_CHAR_DEVICE
#define USE_AESD_CHAR_DEVICE 1
#endif
```

This allows overriding at compile time with `-DUSE_AESD_CHAR_DEVICE=0` if needed,
while defaulting to 1 (char device mode) when built normally.

### Conditional compilation blocks:
- `#if USE_AESD_CHAR_DEVICE` / `#else` / `#endif` wraps:
  - DATA_FILE path definition
  - timer_thread function and its pthread_create/cancel/join
  - remove(DATA_FILE) calls in cleanup() and main()
