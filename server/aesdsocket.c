/**
 * aesdsocket.c - A stream socket server for AESD assignment 5.
 *
 * Opens a stream socket on port 9000. For each accepted connection:
 *   - Receives data and appends each complete line (\n-terminated) to /var/tmp/aesdsocketdata
 *   - After each newline, sends the full contents of /var/tmp/aesdsocketdata back to the client
 *
 * Supports running as a daemon with -d flag.
 * Handles SIGINT and SIGTERM for graceful shutdown (deletes data file, closes socket).
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

#define PORT 9000
#define BACKLOG 10
#define BUF_SIZE 1024
#define DATA_FILE "/var/tmp/aesdsocketdata"

static int server_fd = -1;
static volatile sig_atomic_t caught_signal = 0;

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

/**
 * Send the full contents of DATA_FILE back to the client fd.
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

int main(int argc, char *argv[])
{
    int daemon_mode = 0;

    if (argc > 1 && strcmp(argv[1], "-d") == 0)
        daemon_mode = 1;

    openlog("aesdsocket", LOG_PID, LOG_USER);

    /* Set up signal handlers */
    struct sigaction sa;
    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = signal_handler;
    sigaction(SIGINT, &sa, NULL);
    sigaction(SIGTERM, &sa, NULL);

    /* Create socket */
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

        /* Receive data, append lines to file, send back file after each newline */
        char recv_buf[BUF_SIZE];
        char *line_buf = NULL;
        size_t line_len = 0;

        ssize_t nrecv;
        while ((nrecv = recv(client_fd, recv_buf, sizeof(recv_buf), 0)) > 0) {
            /* Scan for newlines in received data */
            size_t i;
            size_t start = 0;
            for (i = 0; i < (size_t)nrecv; i++) {
                if (recv_buf[i] == '\n') {
                    /* Complete line: line_buf + recv_buf[start..i] + '\n' */
                    size_t chunk_len = i - start + 1;
                    line_buf = realloc(line_buf, line_len + chunk_len);
                    if (!line_buf) {
                        syslog(LOG_ERR, "realloc failed");
                        break;
                    }
                    memcpy(line_buf + line_len, recv_buf + start, chunk_len);
                    line_len += chunk_len;

                    /* Append to file */
                    FILE *fp = fopen(DATA_FILE, "a");
                    if (fp) {
                        fwrite(line_buf, 1, line_len, fp);
                        fclose(fp);
                    }

                    /* Send full file back */
                    send_file_contents(client_fd);

                    /* Reset line buffer */
                    free(line_buf);
                    line_buf = NULL;
                    line_len = 0;
                    start = i + 1;
                }
            }

            /* Buffer remaining partial line */
            if (start < (size_t)nrecv) {
                size_t remaining = nrecv - start;
                line_buf = realloc(line_buf, line_len + remaining);
                if (line_buf) {
                    memcpy(line_buf + line_len, recv_buf + start, remaining);
                    line_len += remaining;
                }
            }
        }

        free(line_buf);
        syslog(LOG_INFO, "Closed connection from %s", client_ip);
        close(client_fd);
    }

    syslog(LOG_INFO, "Caught signal, exiting");
    cleanup();
    return 0;
}
