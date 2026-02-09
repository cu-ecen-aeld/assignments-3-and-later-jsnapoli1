#define _POSIX_C_SOURCE 200809L
#include <arpa/inet.h>
#include <errno.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <signal.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <syslog.h>
#include <unistd.h>

#define AESD_PORT 9000
#define DATA_FILE "/var/tmp/aesdsocketdata"
#define BACKLOG 10
#define RECV_BUFFER_SIZE 1024

static volatile sig_atomic_t exit_requested = 0;

static void signal_handler(int signo)
{
    (void)signo;
    exit_requested = 1;
}

static bool append_packet_to_file(const char *packet, size_t packet_len)
{
    int fd = open(DATA_FILE, O_WRONLY | O_CREAT | O_APPEND, 0644);
    if (fd < 0) {
        syslog(LOG_ERR, "Failed to open data file for append: %s", strerror(errno));
        return false;
    }

    size_t total_written = 0;
    while (total_written < packet_len) {
        ssize_t bytes_written = write(fd, packet + total_written, packet_len - total_written);
        if (bytes_written < 0) {
            syslog(LOG_ERR, "Failed to write packet data: %s", strerror(errno));
            close(fd);
            return false;
        }
        total_written += (size_t)bytes_written;
    }

    close(fd);
    return true;
}

static bool send_file_contents(int client_fd)
{
    int fd = open(DATA_FILE, O_RDONLY);
    if (fd < 0) {
        syslog(LOG_ERR, "Failed to open data file for readback: %s", strerror(errno));
        return false;
    }

    char file_buffer[RECV_BUFFER_SIZE];
    while (true) {
        ssize_t read_bytes = read(fd, file_buffer, sizeof(file_buffer));
        if (read_bytes < 0) {
            syslog(LOG_ERR, "Failed to read data file: %s", strerror(errno));
            close(fd);
            return false;
        }
        if (read_bytes == 0) {
            break;
        }

        size_t sent_total = 0;
        while (sent_total < (size_t)read_bytes) {
            ssize_t sent_bytes = send(client_fd, file_buffer + sent_total,
                                      (size_t)read_bytes - sent_total, 0);
            if (sent_bytes < 0) {
                syslog(LOG_ERR, "Failed to send response data: %s", strerror(errno));
                close(fd);
                return false;
            }
            sent_total += (size_t)sent_bytes;
        }
    }

    close(fd);
    return true;
}

static bool process_client_connection(int client_fd)
{
    char recv_buffer[RECV_BUFFER_SIZE + 1];
    char *packet = NULL;
    size_t packet_size = 0;

    // Pseudocode block: Receive byte stream, build newline-terminated packets, and process each packet.
    // 1) Receive bytes in a loop until client disconnects or fatal error occurs.
    // 2) Grow a heap-backed packet buffer and append incoming bytes.
    // 3) Every time a newline is found, append that full packet to the data file.
    // 4) After each completed packet append, transmit the full data file back to the client.
    // 5) Keep any trailing partial packet bytes for the next recv() call.
    while (true) {
        ssize_t received = recv(client_fd, recv_buffer, RECV_BUFFER_SIZE, 0);
        if (received < 0) {
            if (errno == EINTR) {
                if (exit_requested) {
                    break;
                }
                continue;
            }
            syslog(LOG_ERR, "recv() failed: %s", strerror(errno));
            free(packet);
            return false;
        }

        if (received == 0) {
            break;
        }

        recv_buffer[received] = '\0';

        size_t new_size = packet_size + (size_t)received;
        char *new_packet = realloc(packet, new_size + 1);
        if (new_packet == NULL) {
            syslog(LOG_ERR, "realloc() failed while buffering packet; discarding packet");
            free(packet);
            packet = NULL;
            packet_size = 0;
            continue;
        }

        packet = new_packet;
        memcpy(packet + packet_size, recv_buffer, (size_t)received);
        packet_size = new_size;
        packet[packet_size] = '\0';

        char *line_start = packet;
        char *newline_pos = strchr(line_start, '\n');

        while (newline_pos != NULL) {
            size_t line_len = (size_t)(newline_pos - line_start + 1);
            if (!append_packet_to_file(line_start, line_len)) {
                free(packet);
                return false;
            }

            if (!send_file_contents(client_fd)) {
                free(packet);
                return false;
            }

            line_start = newline_pos + 1;
            newline_pos = strchr(line_start, '\n');
        }

        size_t remaining = packet_size - (size_t)(line_start - packet);
        memmove(packet, line_start, remaining);
        packet_size = remaining;
        packet[packet_size] = '\0';
    }

    free(packet);
    return true;
}

static bool daemonize_if_requested(bool daemon_mode)
{
    if (!daemon_mode) {
        return true;
    }

    // Pseudocode block: Daemonize after bind() succeeded.
    // 1) fork() and let parent exit so child can continue detached.
    // 2) Create a new session with setsid().
    // 3) Move to root directory and reset umask.
    // 4) Redirect stdin/stdout/stderr to /dev/null.
    pid_t pid = fork();
    if (pid < 0) {
        syslog(LOG_ERR, "fork() failed for daemon mode: %s", strerror(errno));
        return false;
    }

    if (pid > 0) {
        exit(EXIT_SUCCESS);
    }

    if (setsid() < 0) {
        syslog(LOG_ERR, "setsid() failed for daemon mode: %s", strerror(errno));
        return false;
    }

    if (chdir("/") < 0) {
        syslog(LOG_ERR, "chdir('/') failed for daemon mode: %s", strerror(errno));
        return false;
    }

    umask(0);

    int null_fd = open("/dev/null", O_RDWR);
    if (null_fd >= 0) {
        dup2(null_fd, STDIN_FILENO);
        dup2(null_fd, STDOUT_FILENO);
        dup2(null_fd, STDERR_FILENO);
        if (null_fd > STDERR_FILENO) {
            close(null_fd);
        }
    }

    return true;
}

int main(int argc, char *argv[])
{
    bool daemon_mode = false;

    if ((argc == 2) && (strcmp(argv[1], "-d") == 0)) {
        daemon_mode = true;
    } else if (argc > 1) {
        fprintf(stderr, "Usage: %s [-d]\n", argv[0]);
        return -1;
    }

    openlog("aesdsocket", LOG_PID, LOG_USER);

    struct sigaction sa;
    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = signal_handler;
    sigemptyset(&sa.sa_mask);

    if (sigaction(SIGINT, &sa, NULL) != 0 || sigaction(SIGTERM, &sa, NULL) != 0) {
        syslog(LOG_ERR, "sigaction failed: %s", strerror(errno));
        closelog();
        return -1;
    }

    // Pseudocode block: Create/bind/listen server socket and then serve clients until shutdown signal.
    // 1) Create AF_INET stream socket.
    // 2) Set SO_REUSEADDR and bind to port 9000 on INADDR_ANY.
    // 3) Start listening for clients.
    // 4) Optionally daemonize only after successful bind/listen steps.
    // 5) accept() clients in a loop, logging connect/disconnect events and handling each client stream.
    int server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd < 0) {
        syslog(LOG_ERR, "socket() failed: %s", strerror(errno));
        closelog();
        return -1;
    }

    int reuse_opt = 1;
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &reuse_opt, sizeof(reuse_opt)) != 0) {
        syslog(LOG_ERR, "setsockopt(SO_REUSEADDR) failed: %s", strerror(errno));
        close(server_fd);
        closelog();
        return -1;
    }

    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    server_addr.sin_port = htons(AESD_PORT);

    if (bind(server_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) != 0) {
        syslog(LOG_ERR, "bind() failed on port %d: %s", AESD_PORT, strerror(errno));
        close(server_fd);
        closelog();
        return -1;
    }

    if (listen(server_fd, BACKLOG) != 0) {
        syslog(LOG_ERR, "listen() failed: %s", strerror(errno));
        close(server_fd);
        closelog();
        return -1;
    }

    if (!daemonize_if_requested(daemon_mode)) {
        close(server_fd);
        closelog();
        return -1;
    }

    while (!exit_requested) {
        struct sockaddr_in client_addr;
        socklen_t client_len = sizeof(client_addr);

        int client_fd = accept(server_fd, (struct sockaddr *)&client_addr, &client_len);
        if (client_fd < 0) {
            if (errno == EINTR && exit_requested) {
                break;
            }
            if (errno == EINTR) {
                continue;
            }
            syslog(LOG_ERR, "accept() failed: %s", strerror(errno));
            continue;
        }

        char client_ip[INET_ADDRSTRLEN];
        const char *addr = inet_ntop(AF_INET, &client_addr.sin_addr, client_ip, sizeof(client_ip));
        if (addr == NULL) {
            strncpy(client_ip, "unknown", sizeof(client_ip));
            client_ip[sizeof(client_ip) - 1] = '\0';
        }

        syslog(LOG_INFO, "Accepted connection from %s", client_ip);
        (void)process_client_connection(client_fd);
        close(client_fd);
        syslog(LOG_INFO, "Closed connection from %s", client_ip);
    }

    syslog(LOG_INFO, "Caught signal, exiting");
    close(server_fd);
    unlink(DATA_FILE);
    closelog();

    return 0;
}
