/**
 * @file aesdchar.c
 * @brief Functions and data related to the AESD char driver implementation
 *
 * Based on the implementation of the "scull" device driver, found in
 * Linux Device Drivers example code.
 *
 * @author Dan Walkes
 * @date 2019-10-22
 * @copyright Copyright (c) 2019
 *
 */

#include <linux/module.h>
#include <linux/init.h>
#include <linux/printk.h>
#include <linux/types.h>
#include <linux/cdev.h>
#include <linux/fs.h> // file_operations
#include <linux/slab.h> // kmalloc, kfree
#include <linux/uaccess.h> // copy_to_user, copy_from_user
#include <linux/mutex.h>
#include "aesdchar.h"
int aesd_major =   0; // use dynamic major
int aesd_minor =   0;

MODULE_AUTHOR("jsnapoli1");
MODULE_LICENSE("Dual BSD/GPL");

struct aesd_dev aesd_device;

/* Store aesd_dev pointer in filp->private_data for read/write access */
int aesd_open(struct inode *inode, struct file *filp)
{
    struct aesd_dev *dev;
    PDEBUG("open");
    dev = container_of(inode->i_cdev, struct aesd_dev, cdev);
    filp->private_data = dev;
    return 0;
}

/* No per-file cleanup needed */
int aesd_release(struct inode *inode, struct file *filp)
{
    PDEBUG("release");
    return 0;
}

/* Read from the circular buffer at the current f_pos offset */
ssize_t aesd_read(struct file *filp, char __user *buf, size_t count,
                loff_t *f_pos)
{
    struct aesd_dev *dev = filp->private_data;
    struct aesd_buffer_entry *entry;
    size_t entry_offset;
    size_t bytes_available;
    size_t bytes_to_copy;
    ssize_t retval = 0;

    PDEBUG("read %zu bytes with offset %lld", count, *f_pos);

    if (mutex_lock_interruptible(&dev->lock))
        return -ERESTARTSYS;

    entry = aesd_circular_buffer_find_entry_offset_for_fpos(&dev->buffer,
                *f_pos, &entry_offset);
    if (!entry) {
        /* No data at this offset — EOF */
        goto out;
    }

    bytes_available = entry->size - entry_offset;
    bytes_to_copy = (count < bytes_available) ? count : bytes_available;

    if (copy_to_user(buf, entry->buffptr + entry_offset, bytes_to_copy)) {
        retval = -EFAULT;
        goto out;
    }

    *f_pos += bytes_to_copy;
    retval = bytes_to_copy;

out:
    mutex_unlock(&dev->lock);
    return retval;
}

/* Write to the partial buffer; commit to circular buffer on newline */
ssize_t aesd_write(struct file *filp, const char __user *buf, size_t count,
                loff_t *f_pos)
{
    struct aesd_dev *dev = filp->private_data;
    char *new_buf;
    ssize_t retval = -ENOMEM;

    PDEBUG("write %zu bytes with offset %lld", count, *f_pos);

    if (mutex_lock_interruptible(&dev->lock))
        return -ERESTARTSYS;

    /* Grow the partial buffer to hold the incoming data */
    new_buf = krealloc(dev->partial_buf, dev->partial_len + count, GFP_KERNEL);
    if (!new_buf)
        goto out;
    dev->partial_buf = new_buf;

    if (copy_from_user(dev->partial_buf + dev->partial_len, buf, count)) {
        retval = -EFAULT;
        goto out;
    }
    dev->partial_len += count;

    /* Check if the partial buffer contains a newline — if so, commit it */
    if (dev->partial_len > 0 && dev->partial_buf[dev->partial_len - 1] == '\n') {
        struct aesd_buffer_entry new_entry;

        new_entry.buffptr = dev->partial_buf;
        new_entry.size = dev->partial_len;

        /* If buffer is full, free the entry that will be evicted */
        if (dev->buffer.full) {
            kfree(dev->buffer.entry[dev->buffer.in_offs].buffptr);
        }

        aesd_circular_buffer_add_entry(&dev->buffer, &new_entry);

        /* Ownership transferred to circular buffer; reset partial state */
        dev->partial_buf = NULL;
        dev->partial_len = 0;
    }

    retval = count;

out:
    mutex_unlock(&dev->lock);
    return retval;
}

/* Seek to a byte position within the circular buffer's logical data stream */
loff_t aesd_llseek(struct file *filp, loff_t offset, int whence)
{
    struct aesd_dev *dev = filp->private_data;
    loff_t new_pos;
    size_t total_size;

    PDEBUG("llseek offset %lld whence %d", offset, whence);

    /**
     * Pseudocode:
     *   1. Acquire the device mutex — we read buffer entry sizes, so we
     *      must prevent concurrent writes from adding or evicting entries
     *      while we compute the total size
     *   2. Compute the total size of all data in the circular buffer by
     *      summing every valid entry's size; this acts as the logical
     *      "file size" for seek purposes
     *   3. Use fixed_size_llseek to handle the three whence modes:
     *        - SEEK_SET: new_pos = offset
     *        - SEEK_CUR: new_pos = current f_pos + offset
     *        - SEEK_END: new_pos = total_size + offset
     *      Validate that the resulting position is in [0, total_size]
     *   4. Release the mutex and return the new position (or error)
     */

    /* Step 1: lock the device to get a consistent view of the buffer */
    if (mutex_lock_interruptible(&dev->lock))
        return -ERESTARTSYS;

    /* Step 2: compute logical file size from circular buffer contents */
    total_size = aesd_circular_buffer_total_size(&dev->buffer);

    /*
     * Step 3: use the kernel's fixed_size_llseek helper
     *
     * fixed_size_llseek handles all three whence values (SEEK_SET,
     * SEEK_CUR, SEEK_END) and validates the resulting position against
     * the provided size.  It updates filp->f_pos on success and returns
     * -EINVAL for out-of-range positions.
     *
     * We use this rather than a manual switch because:
     *   - It correctly handles locking of filp->f_pos via inode->i_mutex
     *     (the kernel takes care of the file position lock internally)
     *   - It matches standard POSIX semantics exactly
     *   - It avoids reimplementing boundary checks that the kernel
     *     already provides and tests
     */
    new_pos = fixed_size_llseek(filp, offset, whence, total_size);

    /* Step 4: release the device mutex */
    mutex_unlock(&dev->lock);

    return new_pos;
}

struct file_operations aesd_fops = {
    .owner =    THIS_MODULE,
    .llseek =   aesd_llseek,
    .read =     aesd_read,
    .write =    aesd_write,
    .open =     aesd_open,
    .release =  aesd_release,
};

static int aesd_setup_cdev(struct aesd_dev *dev)
{
    int err, devno = MKDEV(aesd_major, aesd_minor);

    cdev_init(&dev->cdev, &aesd_fops);
    dev->cdev.owner = THIS_MODULE;
    dev->cdev.ops = &aesd_fops;
    err = cdev_add (&dev->cdev, devno, 1);
    if (err) {
        printk(KERN_ERR "Error %d adding aesd cdev", err);
    }
    return err;
}

int aesd_init_module(void)
{
    dev_t dev = 0;
    int result;
    result = alloc_chrdev_region(&dev, aesd_minor, 1,
            "aesdchar");
    aesd_major = MAJOR(dev);
    if (result < 0) {
        printk(KERN_WARNING "Can't get major %d\n", aesd_major);
        return result;
    }
    memset(&aesd_device, 0, sizeof(struct aesd_dev));

    /* Initialize circular buffer, mutex, and partial write state */
    aesd_circular_buffer_init(&aesd_device.buffer);
    mutex_init(&aesd_device.lock);

    result = aesd_setup_cdev(&aesd_device);

    if (result) {
        unregister_chrdev_region(dev, 1);
    }
    return result;
}

void aesd_cleanup_module(void)
{
    uint8_t index;
    struct aesd_buffer_entry *entry;
    dev_t devno = MKDEV(aesd_major, aesd_minor);

    cdev_del(&aesd_device.cdev);

    /* Free all entries in the circular buffer */
    AESD_CIRCULAR_BUFFER_FOREACH(entry, &aesd_device.buffer, index) {
        if (entry->buffptr)
            kfree(entry->buffptr);
    }

    /* Free any pending partial write */
    kfree(aesd_device.partial_buf);

    unregister_chrdev_region(devno, 1);
}

module_init(aesd_init_module);
module_exit(aesd_cleanup_module);
