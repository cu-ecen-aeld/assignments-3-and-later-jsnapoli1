# Plan: Circular Buffer Implementation (Assignment 7/8)

## Overview

Implement a circular (ring) buffer in `aesd-char-driver/aesd-circular-buffer.c` that stores up to `AESDCHAR_MAX_WRITE_OPERATIONS_SUPPORTED` (10) entries. The buffer must support:
- Initialization to empty state
- Adding entries with automatic overwrite of oldest when full
- Finding a specific byte position across all concatenated entries

The code must compile in both kernel space (`__KERNEL__` defined) and user space (for unit testing).

## Data Structures (from header — read-only)

- `aesd_buffer_entry`: holds a `const char *buffptr` and a `size_t size`
- `aesd_circular_buffer`: fixed array of 10 `aesd_buffer_entry`, with `uint8_t in_offs` (write position), `uint8_t out_offs` (read position), and `bool full` (disambiguates full vs. empty when `in_offs == out_offs`)

## Constraints

1. Do NOT modify `aesd-circular-buffer.h`
2. Memory management is the caller's responsibility
3. Locking is the caller's responsibility
4. Preserve `#ifdef __KERNEL__` guard for dual compilation

## Implementation Plan

### Block 1: `aesd_circular_buffer_init`

**Goal**: Initialize buffer to empty state.

**Pseudocode**:
```
function init(buffer):
    zero out all bytes of the buffer struct (memset to 0)
    // This sets: all entry pointers to NULL, all sizes to 0,
    //            in_offs = 0, out_offs = 0, full = false
```

**Notes**: The existing stub already has `memset(buffer, 0, sizeof(...))` — this is correct and complete. Verify it handles all fields and commit.

### Block 2: `aesd_circular_buffer_add_entry`

**Goal**: Write a new entry at `in_offs`. If the buffer is full, overwrite the oldest entry and advance `out_offs`.

**Pseudocode**:
```
function add_entry(buffer, add_entry):
    // Copy the entry data (buffptr and size) into the slot at in_offs
    buffer->entry[buffer->in_offs] = *add_entry

    // If the buffer was already full before this write,
    // the oldest entry just got overwritten, so advance out_offs
    if buffer->full:
        buffer->out_offs = (buffer->out_offs + 1) % MAX_WRITE_OPS

    // Advance the write pointer
    buffer->in_offs = (buffer->in_offs + 1) % MAX_WRITE_OPS

    // Check if the buffer is now full
    // (write pointer caught up to read pointer)
    if buffer->in_offs == buffer->out_offs:
        buffer->full = true
```

**Key decisions**:
- The entry struct is copied by value (shallow copy of pointer + size), matching the caller-owns-memory contract
- `full` flag must be checked BEFORE advancing `in_offs` to correctly detect overwrite
- Modular arithmetic with `% AESDCHAR_MAX_WRITE_OPERATIONS_SUPPORTED` wraps indices

### Block 3: `aesd_circular_buffer_find_entry_offset_for_fpos`

**Goal**: Treat all buffer entries as one concatenated byte stream. Given a byte position `char_offset`, find which entry contains it and the offset within that entry.

**Pseudocode**:
```
function find_entry_offset_for_fpos(buffer, char_offset, entry_offset_byte_rtn):
    // Determine number of entries currently in the buffer
    // If full, there are MAX entries; otherwise compute from offsets

    // Start iterating from out_offs (oldest entry)
    // For each entry, check if char_offset falls within this entry's byte range
    //   If char_offset < current_entry.size:
    //       Set *entry_offset_byte_rtn = char_offset (remaining offset within entry)
    //       Return pointer to this entry
    //   Else:
    //       Subtract current_entry.size from char_offset
    //       Advance to next entry

    // If we exhaust all entries without finding the offset, return NULL
```

**Edge cases**:
- Empty buffer (not full AND `in_offs == out_offs`): return NULL immediately
- Offset beyond total stored bytes: return NULL after iterating all entries
- Full buffer where `in_offs == out_offs`: the `full` flag disambiguates — iterate all 10 entries

## Testing Strategy

1. Run `./unit-test.sh` from repo root after each block implementation
2. The test writes 10 entries ("write1\n" through "write10\n", each 7 or 8 bytes)
3. Verifies byte offsets 0, 7, 14, 21, ... map to correct entries
4. Verifies offset 71 (one past end) returns NULL
5. Adds an 11th entry, verifying oldest is evicted and offsets shift
6. Run `./full-test.sh` for complete assignment validation

## Commit Strategy

One commit per function block, plus test pass confirmation and documentation commits.
