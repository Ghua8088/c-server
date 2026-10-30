#include "connection.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#if defined(_WIN32)
#include <winsock2.h>
#include <ws2tcpip.h>

#ifdef _MSC_VER
    #pragma comment(lib, "ws2_32.lib")
#endif

/* Adapter callbacks for platform_process_connection */
static int win32_read(void *ctx, void *buf, size_t max_len) {
    SOCKET s = *(SOCKET *)ctx;
    return recv(s, (char *)buf, (int)max_len, 0);
}

static int win32_write(void *ctx, const void *buf, size_t len) {
    SOCKET s = *(SOCKET *)ctx;
    return send(s, (const char *)buf, (int)len, 0);
}

int platform_listen_win32(app_t *app, int port) {
    WSADATA wsa_data;
    if (WSAStartup(MAKEWORD(2, 2), &wsa_data) != 0) {
        fprintf(stderr, "[Winsock] WSAStartup failed.\n");
        return -1;
    }

    SOCKET server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd == INVALID_SOCKET) {
        fprintf(stderr, "[Winsock] Failed to create socket.\n");
        WSACleanup();
        return -1;
    }

    int opt = 1;
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, (const char *)&opt, sizeof(opt));

    struct sockaddr_in address;
    memset(&address, 0, sizeof(address));
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons((uint16_t)port);

    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
        fprintf(stderr, "[Winsock] Bind failed on port %d.\n", port);
        closesocket(server_fd);
        WSACleanup();
        return -1;
    }

    if (listen(server_fd, 128) < 0) {
        fprintf(stderr, "[Winsock] Listen failed.\n");
        closesocket(server_fd);
        WSACleanup();
        return -1;
    }

    printf("\n==================================================\n");
    printf("  [Driver: Winsock] Listening on http://localhost:%d\n", port);
    printf("  Ready for requests! (Press Ctrl+C to stop)\n");
    printf("==================================================\n\n");
    fflush(stdout);

    while (1) {
        struct sockaddr_in client_addr;
        int client_len = sizeof(client_addr);
        SOCKET client_fd = accept(server_fd, (struct sockaddr *)&client_addr, &client_len);

        if (client_fd == INVALID_SOCKET) {
            continue;
        }

        /* Process request through shared Sans-I/O pipeline */
        platform_process_connection(app, &client_fd, win32_read, win32_write);

        /* Graceful TCP close */
        shutdown(client_fd, SD_SEND);
        closesocket(client_fd);
    }

    closesocket(server_fd);
    WSACleanup();
    return 0;
}

#endif /* _WIN32 */

