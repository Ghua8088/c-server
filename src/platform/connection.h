#ifndef CSERVER_PLATFORM_CONNECTION_H
#define CSERVER_PLATFORM_CONNECTION_H

#include "../../include/server.h"
#include "../../include/context.h"
#include "../../include/http_proto.h"
#include <stddef.h>

/**
 * Sans-I/O Connection Pipeline
 * Decouples request lifecycle from OS sockets.
 * Uses function pointers so any stream (Winsock, POSIX, WASI stdin/stdout) works cleanly.
 */

typedef int (*platform_read_fn)(void *ctx, void *buf, size_t max_len);
typedef int (*platform_write_fn)(void *ctx, const void *buf, size_t len);

void platform_process_connection(app_t *app, void *stream_ctx,
                                 platform_read_fn read_cb,
                                 platform_write_fn write_cb);

#endif /* CSERVER_PLATFORM_CONNECTION_H */

