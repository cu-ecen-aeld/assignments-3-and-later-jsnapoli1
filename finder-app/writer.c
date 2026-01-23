/*
 * writer.c
 *
 * Creates a new file with name and path writefile with content writestr,
 * overwriting any existing file. Directory creation is the caller's
 * responsibility.
 */

#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <syslog.h>
#include <unistd.h>

static int write_all(int fd, const char *buffer, size_t length)
{
    size_t total = 0;

    while (total < length) {
        ssize_t written = write(fd, buffer + total, length - total);
        if (written < 0) {
            return -1;
        }
        total += (size_t)written;
    }

    return 0;
}

int main(int argc, char *argv[])
{
    const char *writefile = NULL;
    const char *writestr = NULL;
    int fd = -1;
    int status = 0;

    openlog("writer", LOG_PID, LOG_USER);

    if (argc != 3 || argv[1][0] == '\0' || argv[2][0] == '\0') {
        syslog(LOG_ERR, "Error: Two arguments required: <writefile> <writestr>");
        fprintf(stderr, "Error: Two arguments required: <writefile> <writestr>\n");
        closelog();
        return 1;
    }

    writefile = argv[1];
    writestr = argv[2];

    syslog(LOG_DEBUG, "Writing %s to %s", writestr, writefile);

    fd = open(writefile, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (fd == -1) {
        syslog(LOG_ERR, "Error: Could not write to file %s: %s", writefile, strerror(errno));
        fprintf(stderr, "Error: Could not write to file %s\n", writefile);
        closelog();
        return 1;
    }

    if (write_all(fd, writestr, strlen(writestr)) != 0) {
        syslog(LOG_ERR, "Error: Could not write to file %s: %s", writefile, strerror(errno));
        fprintf(stderr, "Error: Could not write to file %s\n", writefile);
        status = 1;
    }

    /* If f* calls were allowed, could be worth putting an fsync here to ensure writing the file to physical media. */
    if (close(fd) != 0) {
        syslog(LOG_ERR, "Error: Could not close file %s: %s", writefile, strerror(errno));
        fprintf(stderr, "Error: Could not close file %s\n", writefile);
        status = 1;
    }

    /* A sync command could have been put here, but is unfavorable on large systems that have lots of buffers. */

    closelog();
    return status;
}
