/**
 * @file aesd-circular-buffer.c
 * @brief Functions and data related to a circular buffer imlementation
 *
 * @author Dan Walkes
 * @date 2020-03-01
 * @copyright Copyright (c) 2020
 *
 */

#ifdef __KERNEL__
#include <linux/string.h>
#else
#include <string.h>
#endif

#include "aesd-circular-buffer.h"

/**
 * @param buffer the buffer to search for corresponding offset.  Any necessary locking must be performed by caller.
 * @param char_offset the position to search for in the buffer list, describing the zero referenced
 *      character index if all buffer strings were concatenated end to end
 * @param entry_offset_byte_rtn is a pointer specifying a location to store the byte of the returned aesd_buffer_entry
 *      buffptr member corresponding to char_offset.  This value is only set when a matching char_offset is found
 *      in aesd_buffer.
 * @return the struct aesd_buffer_entry structure representing the position described by char_offset, or
 * NULL if this position is not available in the buffer (not enough data is written).
 */
struct aesd_buffer_entry *aesd_circular_buffer_find_entry_offset_for_fpos(struct aesd_circular_buffer *buffer,
            size_t char_offset, size_t *entry_offset_byte_rtn )
{
    /**
    * TODO: implement per description
    */
    return NULL;
}

/**
* Adds entry @param add_entry to @param buffer in the location specified in buffer->in_offs.
* If the buffer was already full, overwrites the oldest entry and advances buffer->out_offs to the
* new start location.
* Any necessary locking must be handled by the caller
* Any memory referenced in @param add_entry must be allocated by and/or must have a lifetime managed by the caller.
*/
void aesd_circular_buffer_add_entry(struct aesd_circular_buffer *buffer, const struct aesd_buffer_entry *add_entry)
{
    /**
     * Pseudocode:
     *   1. Write the new entry into the slot at in_offs (shallow copy of
     *      buffptr pointer and size — caller owns the underlying memory)
     *   2. If buffer was already full before this write, the oldest entry
     *      at out_offs was just overwritten, so advance out_offs by one
     *      (wrapping around with modulo)
     *   3. Advance in_offs to the next slot (wrapping around with modulo)
     *   4. If in_offs now equals out_offs, the buffer has become full
     */

    /* Step 1: copy the entry into the current write slot */
    buffer->entry[buffer->in_offs] = *add_entry;

    /* Step 2: if full, oldest entry was overwritten — advance the read pointer */
    if (buffer->full) {
        buffer->out_offs = (buffer->out_offs + 1) % AESDCHAR_MAX_WRITE_OPERATIONS_SUPPORTED;
    }

    /* Step 3: advance the write pointer to the next slot */
    buffer->in_offs = (buffer->in_offs + 1) % AESDCHAR_MAX_WRITE_OPERATIONS_SUPPORTED;

    /* Step 4: detect whether the buffer is now full (write caught up to read) */
    buffer->full = (buffer->in_offs == buffer->out_offs);
}

/**
* Initializes the circular buffer described by @param buffer to an empty struct
*/
void aesd_circular_buffer_init(struct aesd_circular_buffer *buffer)
{
    /**
     * Pseudocode:
     *   Zero out the entire buffer struct so that:
     *     - All entry[].buffptr pointers become NULL
     *     - All entry[].size values become 0
     *     - in_offs = 0  (next write goes to slot 0)
     *     - out_offs = 0 (next read comes from slot 0)
     *     - full = false (buffer starts empty)
     */
    memset(buffer, 0, sizeof(struct aesd_circular_buffer));
}
