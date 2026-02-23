/**
 * aesdsocket.c - Multi-threaded stream socket server for AESD Assignment 6 Part 1.
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

struct thread_entry {
    pthread_t tid;
    int client_fd;
    volatile bool complete;
    SLIST_ENTRY(thread_entry) entries;
};

SLIST_HEAD(thread_list, thread_entry) thread_head = SLIST_HEAD_INITIALIZER(thread_head);

static pthread_mutex_t data_mutex = PTHREAD_MUTEX_INITIALIZER;

static void signal_handler(int signo)
{
    caught_signal = signo;
}

static void cleanup(void)
{
    if (server_fd >= 0)
        close(server_fd);
    remove(DATA_FILE);
    closelog();
}

static int send_file_contents(int client_fd)
{
    int fd = open(DATA_FILE, O_RDONLY);
    if (fd < 0)
        return -1;

    char buf[BUF_SIZE];
    ssize_t nread;
    while ((nread = read(fd, buf, sizeof(buf))) > 0) {
        ssize_t total_sent = 0;
        while (total_sent < nread) {
            ssize_t sent = send(client_fd, buf + total_sent, nread - total_sent, 0);
            if (sent <= 0) {
                close(fd);
                return -1;
            }
            total_sent += sent;
        }
    }
    close(fd);
    return 0;
}

static void *connection_thread(void *arg)
{
    struct thread_entry *entry = (struct thread_entry *)arg;
    int client_fd = entry->client_fd;

    char recv_buf[BUF_SIZE];
    char *line_buf = NULL;
    size_t line_len = 0;

    ssize_t nrecv;
    while ((nrecv = recv(client_fd, recv_buf, sizeof(recv_buf), 0)) > 0) {
        size_t i;
        size_t start = 0;
        for (i = 0; i < (size_t)nrecv; i++) {
            if (recv_buf[i] == '\n') {
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

                pthread_mutex_lock(&data_mutex);

                /* Use file descriptor with O_APPEND to ensure atomicity */
                int fd = open(DATA_FILE, O_WRONLY | O_CREAT | O_APPEND, 0644);
                if (fd >= 0) {
                    size_t total_written = 0;
                    while (total_written < line_len) {
                        ssize_t written = write(fd, line_buf + total_written, line_len - total_written);
                        if (written < 0) {
                            if (errno == EINTR) {
                                continue;
                            }
                            syslog(LOG_ERR, "write failed: %s", strerror(errno));
                            break;
                        }
                        total_written += written;
                    }
                    if (total_written != line_len) {
                        syslog(LOG_ERR, "incomplete write: %zu/%zu bytes", total_written, line_len);
                    }
                    close(fd);
                }

                send_file_contents(client_fd);

                pthread_mutex_unlock(&data_mutex);

                free(line_buf);
                line_buf = NULL;
                line_len = 0;
                start = i + 1;
            }
        }

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
    entry->complete = true;
    return NULL;
}

static void *timer_thread(void *arg)
{
    (void)arg;
    struct timespec ts = {10, 0};

    while (!caught_signal) {
        clock_nanosleep(CLOCK_MONOTONIC, 0, &ts, NULL);
        if (caught_signal)
            break;

        time_t now = time(NULL);
        struct tm tm_info;
        localtime_r(&now, &tm_info);
        char timebuf[128];
        strftime(timebuf, sizeof(timebuf),
                 "timestamp:%a, %d %b %Y %H:%M:%S %z\n", &tm_info);

        pthread_mutex_lock(&data_mutex);
        int fd = open(DATA_FILE, O_WRONLY | O_CREAT | O_APPEND, 0644);
        if (fd >= 0) {
            write(fd, timebuf, strlen(timebuf));
            close(fd);
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

    remove(DATA_FILE);

    struct sigaction sa;
    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = signal_handler;
    sigaction(SIGINT, &sa, NULL);
    sigaction(SIGTERM, &sa, NULL);

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

    if (daemon_mode) {
        pid_t pid = fork();
        if (pid < 0) {
            syslog(LOG_ERR, "fork: %s", strerror(errno));
            cleanup();
            return -1;
        }
        if (pid > 0)
            exit(0);
        setsid();
        chdir("/");
        close(STDIN_FILENO);
        close(STDOUT_FILENO);
        close(STDERR_FILENO);
    }

    syslog(LOG_INFO, "Listening on port %d", PORT);

    pthread_t timer_tid;
    pthread_create(&timer_tid, NULL, timer_thread, NULL);

    while (1) {
        struct sockaddr_in client_addr;
        socklen_t client_len = sizeof(client_addr);

        if (caught_signal) {
            int flags = fcntl(server_fd, F_GETFL, 0);
            fcntl(server_fd, F_SETFL, flags | O_NONBLOCK);
        }

        int client_fd = accept(server_fd, (struct sockaddr *)&client_addr, &client_len);
        
        if (client_fd < 0) {
            if (caught_signal) {
                if (errno == EWOULDBLOCK || errno == EAGAIN)
                    break;
            }
            if (errno == EINTR)
                continue;
            if (errno == EWOULDBLOCK || errno == EAGAIN)
                break;
            syslog(LOG_ERR, "accept: %s", strerror(errno));
            continue;
        }

        char *client_ip = inet_ntoa(client_addr.sin_addr);
        syslog(LOG_INFO, "Accepted connection from %s", client_ip);

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

    if (server_fd >= 0) {
        close(server_fd);
        server_fd = -1;
    }

    pthread_cancel(timer_tid);
    pthread_join(timer_tid, NULL);

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
