/*
 * aesd_ioctl.h
 *
 * AESD specific ioctl command definitions shared between kernel driver
 * and userspace applications.
 *
 *  Created on: Oct 23, 2019
 *      Author: Dan Walkes
 *
 *  Updated for assignment 9 ioctl support.
 */

#ifndef AESD_IOCTL_H
#define AESD_IOCTL_H

#ifdef __KERNEL__
#include <asm/ioctl.h>
#include <linux/types.h>
#else
#include <sys/ioctl.h>
#include <stdint.h>
#endif

/*
 * Pseudocode Block 1 — Define the ioctl interface contract
 *
 *   DEFINE a magic number unique to this driver to avoid ioctl collisions
 *   DEFINE a struct that carries two seek parameters:
 *     - write_cmd:        which command (zero-indexed) in the circular buffer
 *     - write_cmd_offset: byte offset within that command (zero-indexed)
 *   DEFINE the AESDCHAR_IOCSEEKTO command number using _IOW so the kernel
 *     knows this ioctl writes a struct from user to kernel
 */

/**
 * A magic number unique to this driver, used as the first argument to the
 * _IOW macro.  Prevents accidental collisions with ioctl numbers from other
 * drivers.  0x16 is chosen by course convention.
 */
#define AESD_IOC_MAGIC 0x16

/**
 * Describes the target position for an ioctl-based seek.
 * @write_cmd        Zero-referenced index of the write command to seek into,
 *                   relative to the commands currently stored in the circular
 *                   buffer (0 = oldest stored command).
 * @write_cmd_offset Zero-referenced byte offset within that write command.
 */
struct aesd_seekto {
    uint32_t write_cmd;
    uint32_t write_cmd_offset;
};

/*
 * AESDCHAR_IOCSEEKTO — ioctl command number
 *
 * _IOW(magic, ordinal, type) builds a unique 32-bit command code:
 *   - direction bits  = _IOC_WRITE  (data flows user -> kernel)
 *   - magic byte      = AESD_IOC_MAGIC (0x16)
 *   - ordinal         = 0 (first and only ioctl for this driver)
 *   - size field      = sizeof(struct aesd_seekto)
 *
 * The kernel's ioctl dispatch uses these fields to validate direction and
 * size before the driver handler runs.
 */
#define AESDCHAR_IOCSEEKTO _IOW(AESD_IOC_MAGIC, 0, struct aesd_seekto)

/**
 * Maximum ioctl ordinal defined by this driver.  Used for range checks
 * in the ioctl handler (cmd ordinal must be <= this value).
 */
#define AESDCHAR_IOC_MAXNR 0

#endif /* AESD_IOCTL_H */
