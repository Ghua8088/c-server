#include "connection.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#if !defined(_WIN32) && !defined(__wasi__)
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>

/* Adapter callbacks for platform_process_connection */
static int posix_read(void *ctx, void *buf, size_t max_len) {
    int fd = *(int *)ctx;
    return (int)read(fd, buf, max_len);
}

static int posix_write(void *ctx, const void *buf, size_t len) {
    int fd = *(int *)ctx;
    return (int)write(fd, buf, len);
}

int platform_listen_posix(app_t *app, int port) {
    int server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd < 0) {
        perror("[POSIX] Failed to create socket");
        return -1;
    }

    int opt = 1;
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    struct sockaddr_in address;
    memset(&address, 0, sizeof(address));
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons((uint16_t)port);

    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
        perror("[POSIX] Bind failed");
        close(server_fd);
        return -1;
    }

    if (listen(server_fd, 128) < 0) {
        perror("[POSIX] Listen failed");
        close(server_fd);
        return -1;
    }

    printf("\n==================================================\n");
    printf("  [Driver: POSIX] Listening on http://localhost:%d\n", port);
    printf("  Ready for requests! (Press Ctrl+C to stop)\n");
    printf("==================================================\n\n");
    fflush(stdout);

    while (1) {
        struct sockaddr_in client_addr;
        socklen_t client_len = sizeof(client_addr);
        int client_fd = accept(server_fd, (struct sockaddr *)&client_addr, &client_len);

        if (client_fd < 0) {
            continue;
        }

        /* Process request through shared Sans-I/O pipeline */
        platform_process_connection(app, &client_fd, posix_read, posix_write);

        /* Graceful TCP close */
        shutdown(client_fd, SHUT_WR);
        close(client_fd);
    }

    close(server_fd);
    return 0;
}

#endif /* !defined(_WIN32) && !defined(__wasi__) */

