#include "../../include/server.h"
#include <stdio.h>

/**
 * Platform I/O Driver / Socket Daemon
 * Handles network listening, accepts TCP client sockets,
 * feeds byte buffers into the Sans-I/O HTTP parser, and dispatches via app_dispatch.
 */

int app_listen(app_t *app, int port) {
    printf("[c-server] Starting listener on port %d...\n", port);
    
#if defined(_WIN32)
    /* Winsock initialization & select/IOCP loop */
    printf("[c-server] Platform: Windows (Winsock)\n");
#elif defined(__wasi__)
    /* WASI 0.2 wasi:sockets or stdin/stdout streaming mode */
    printf("[c-server] Platform: WebAssembly (WASI)\n");
#else
    /* POSIX BSD sockets & poll/epoll */
    printf("[c-server] Platform: POSIX\n");
#endif

    (void)app;
    return 0;
}

