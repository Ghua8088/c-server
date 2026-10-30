#include "connection.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#if defined(__wasi__)

/* In WASM/WASI, input streams from stdin and output writes to stdout,
 * or attaches to WASI 0.2 wasi:sockets. */
typedef struct {
    FILE *in;
    FILE *out;
} wasi_stream_t;

static int wasi_read(void *ctx, void *buf, size_t max_len) {
    wasi_stream_t *stream = (wasi_stream_t *)ctx;
    return (int)fread(buf, 1, max_len, stream->in);
}

static int wasi_write(void *ctx, const void *buf, size_t len) {
    wasi_stream_t *stream = (wasi_stream_t *)ctx;
    size_t written = fwrite(buf, 1, len, stream->out);
    fflush(stream->out);
    return (int)written;
}

int platform_listen_wasi(app_t *app, int port) {
    (void)port;
    /* WASI Stream Processing: process request from stdin and output to stdout */
    wasi_stream_t stream = {
        .in = stdin,
        .out = stdout
    };

    platform_process_connection(app, &stream, wasi_read, wasi_write);
    return 0;
}

#endif /* __wasi__ */

