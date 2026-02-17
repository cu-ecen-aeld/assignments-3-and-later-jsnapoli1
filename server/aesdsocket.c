/**
 * aesdsocket.c - Multi-threaded stream socket server for AESD Assignment 6 Part 1.
 *
 * Opens a stream socket on port 9000. For each accepted connection:
 *   - Spawns a joinable thread to handle the client
 *   - Receives data and appends each complete line (\n-terminated) to /var/tmp/aesdsocketdata
 *   - After each newline, sends the full contents of the data file back to the client
 *
 * A timer thread appends an RFC2822 timestamp every 10 seconds.
 * All file access is protected by a pthread mutex.
 * Supports running as a daemon with -d flag.
 * Handles SIGINT and SIGTERM for graceful shutdown.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <signal.h>
#include <syslog.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <pthread.h>
#include <sys/queue.h>
#include <time.h>
#include <stdbool.h>

#define PORT 9000
#define BACKLOG 10
#define BUF_SIZE 1024
#define DATA_FILE "/var/tmp/aesdsocketdata"

static int server_fd = -1;
static volatile sig_atomic_t caught_signal = 0;

/* Thread entry for singly-linked list management
 * pseudocode: struct { tid, client_fd, complete flag, list linkage } */
struct thread_entry {
    pthread_t tid;
    int client_fd;
    volatile bool complete;
    SLIST_ENTRY(thread_entry) entries;
};

/* SLIST head for tracking all spawned threads */
SLIST_HEAD(thread_list, thread_entry) thread_head = SLIST_HEAD_INITIALIZER(thread_head);

/* Global mutex protecting all reads/writes to DATA_FILE */
static pthread_mutex_t data_mutex = PTHREAD_MUTEX_INITIALIZER;

/* Signal handler: set flag and close server_fd to unblock accept() */
static void signal_handler(int signo)
{
    caught_signal = signo;
    if (server_fd >= 0)
        close(server_fd);
    server_fd = -1;
}

/* Cleanup: remove data file and close syslog */
static void cleanup(void)
{
    if (server_fd >= 0)
        close(server_fd);
    remove(DATA_FILE);
    closelog();
}

/**
 * Send the full contents of DATA_FILE back to the client fd.
 * Caller must hold data_mutex.
 *
 * pseudocode: open file "r" -> fread in chunks -> send each chunk -> close
 */
static int send_file_contents(int client_fd)
{
    FILE *fp = fopen(DATA_FILE, "r");
    if (!fp)
        return -1;

    char buf[BUF_SIZE];
    size_t nread;
    while ((nread = fread(buf, 1, sizeof(buf), fp)) > 0) {
        size_t total_sent = 0;
        while (total_sent < nread) {
            ssize_t sent = send(client_fd, buf + total_sent, nread - total_sent, 0);
            if (sent <= 0) {
                fclose(fp);
                return -1;
            }
            total_sent += sent;
        }
    }
    fclose(fp);
    return 0;
}

/**
 * Per-connection thread function.
 *
 * pseudocode:
 *   recv loop: buffer input, scan for newlines
 *   on newline: lock mutex -> append packet to file -> send file back -> unlock
 *   on close/error: break, close client_fd, mark complete
 */
static void *connection_thread(void *arg)
{
    struct thread_entry *entry = (struct thread_entry *)arg;
    int client_fd = entry->client_fd;

    char recv_buf[BUF_SIZE];
    char *line_buf = NULL;
    size_t line_len = 0;

    ssize_t nrecv;
    /* Receive loop: read data from client until connection closes */
    while ((nrecv = recv(client_fd, recv_buf, sizeof(recv_buf), 0)) > 0) {
        /* Scan received data for newline characters */
        size_t i;
        size_t start = 0;
        for (i = 0; i < (size_t)nrecv; i++) {
            if (recv_buf[i] == '\n') {
                /* Complete line found: accumulate chunk into line_buf */
                size_t chunk_len = i - start + 1;
                char *tmp = realloc(line_buf, line_len + chunk_len);
                if (!tmp) {
                    syslog(LOG_ERR, "realloc failed");
                    free(line_buf);
                    line_buf = NULL;
                    line_len = 0;
                    goto done;
                }
                line_buf = tmp;
                memcpy(line_buf + line_len, recv_buf + start, chunk_len);
                line_len += chunk_len;

                /* Lock mutex -> append line to DATA_FILE -> send file -> unlock */
                pthread_mutex_lock(&data_mutex);

                FILE *fp = fopen(DATA_FILE, "a");
                if (fp) {
                    fwrite(line_buf, 1, line_len, fp);
                    fclose(fp);
                }

                /* Send full file contents back to client (still under lock) */
                send_file_contents(client_fd);

                pthread_mutex_unlock(&data_mutex);

                /* Reset line buffer for next packet */
                free(line_buf);
                line_buf = NULL;
                line_len = 0;
                start = i + 1;
            }
        }

        /* Buffer remaining partial line data (no newline yet) */
        if (start < (size_t)nrecv) {
            size_t remaining = nrecv - start;
            char *tmp = realloc(line_buf, line_len + remaining);
            if (tmp) {
                line_buf = tmp;
                memcpy(line_buf + line_len, recv_buf + start, remaining);
                line_len += remaining;
            }
        }
    }

done:
    free(line_buf);
    close(client_fd);
    /* Mark thread as complete so main loop can join and free */
    entry->complete = true;
    return NULL;
}

/**
 * Timer thread: appends RFC2822 timestamp to DATA_FILE every 10 seconds.
 *
 * pseudocode:
 *   loop while !caught_signal:
 *     sleep 10s via clock_nanosleep
 *     format timestamp as "timestamp:<RFC2822>\n"
 *     lock mutex -> append to file -> unlock
 */
static void *timer_thread(void *arg)
{
    (void)arg;
    struct timespec ts = {10, 0};

    while (!caught_signal) {
        /* Sleep 10 seconds (interruptible by signal) */
        clock_nanosleep(CLOCK_MONOTONIC, 0, &ts, NULL);
        if (caught_signal)
            break;

        /* Format current time as RFC2822 */
        time_t now = time(NULL);
        struct tm tm_info;
        localtime_r(&now, &tm_info);
        char timebuf[128];
        strftime(timebuf, sizeof(timebuf),
                 "timestamp:%a, %d %b %Y %H:%M:%S %z\n", &tm_info);

        /* Lock mutex -> append timestamp to DATA_FILE -> unlock */
        pthread_mutex_lock(&data_mutex);
        FILE *f = fopen(DATA_FILE, "a");
        if (f) {
            fputs(timebuf, f);
            fclose(f);
        }
        pthread_mutex_unlock(&data_mutex);
    }
    return NULL;
}

int main(int argc, char *argv[])
{
    int daemon_mode = 0;

    if (argc > 1 && strcmp(argv[1], "-d") == 0)
        daemon_mode = 1;

    openlog("aesdsocket", LOG_PID, LOG_USER);

    /* Remove any stale data file from previous runs */
    remove(DATA_FILE);

    /* Set up signal handlers for graceful shutdown */
    struct sigaction sa;
    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = signal_handler;
    sigaction(SIGINT, &sa, NULL);
    sigaction(SIGTERM, &sa, NULL);

    /* Create and configure server socket */
    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd < 0) {
        syslog(LOG_ERR, "socket: %s", strerror(errno));
        return -1;
    }

    int optval = 1;
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval));

    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(PORT);

    if (bind(server_fd, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        syslog(LOG_ERR, "bind: %s", strerror(errno));
        cleanup();
        return -1;
    }

    if (listen(server_fd, BACKLOG) < 0) {
        syslog(LOG_ERR, "listen: %s", strerror(errno));
        cleanup();
        return -1;
    }

    /* Daemonize after bind/listen so port errors are caught early */
    if (daemon_mode) {
        pid_t pid = fork();
        if (pid < 0) {
            syslog(LOG_ERR, "fork: %s", strerror(errno));
            cleanup();
            return -1;
        }
        if (pid > 0)
            exit(0); /* Parent exits */
        setsid();
        chdir("/");
        close(STDIN_FILENO);
        close(STDOUT_FILENO);
        close(STDERR_FILENO);
    }

    syslog(LOG_INFO, "Listening on port %d", PORT);

    /* Start timer thread (appends timestamps every 10s) */
    pthread_t timer_tid;
    pthread_create(&timer_tid, NULL, timer_thread, NULL);

    /* pseudocode: main accept loop - spawn thread per connection, reap completed */
    while (!caught_signal) {
        struct sockaddr_in client_addr;
        socklen_t client_len = sizeof(client_addr);

        int client_fd = accept(server_fd, (struct sockaddr *)&client_addr, &client_len);
        if (client_fd < 0) {
            if (caught_signal)
                break;
            syslog(LOG_ERR, "accept: %s", strerror(errno));
            continue;
        }

        char *client_ip = inet_ntoa(client_addr.sin_addr);
        syslog(LOG_INFO, "Accepted connection from %s", client_ip);

        /* Allocate thread_entry, insert into SLIST, spawn thread */
        struct thread_entry *entry = malloc(sizeof(*entry));
        if (!entry) {
            syslog(LOG_ERR, "malloc failed");
            close(client_fd);
            continue;
        }
        entry->client_fd = client_fd;
        entry->complete = false;
        SLIST_INSERT_HEAD(&thread_head, entry, entries);
        pthread_create(&entry->tid, NULL, connection_thread, entry);

        /* Reap completed threads: iterate list, join and free completed ones
         * Using manual iteration since SLIST_FOREACH_SAFE may not be available */
        struct thread_entry *e = SLIST_FIRST(&thread_head);
        while (e != NULL) {
            struct thread_entry *next = SLIST_NEXT(e, entries);
            if (e->complete) {
                pthread_join(e->tid, NULL);
                SLIST_REMOVE(&thread_head, e, thread_entry, entries);
                free(e);
            }
            e = next;
        }
    }

    syslog(LOG_INFO, "Caught signal, exiting");

    /* Shutdown: join timer thread */
    pthread_join(timer_tid, NULL);

    /* Join all remaining connection threads */
    while (!SLIST_EMPTY(&thread_head)) {
        struct thread_entry *e = SLIST_FIRST(&thread_head);
        pthread_join(e->tid, NULL);
        SLIST_REMOVE_HEAD(&thread_head, entries);
        free(e);
    }

    pthread_mutex_destroy(&data_mutex);
    cleanup();
    return 0;
}
