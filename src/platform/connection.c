#include "connection.h"
#include <string.h>
#include <stdio.h>

#define RECV_BUFFER_SIZE 65536
#define ARENA_CAPACITY   65536

void platform_process_connection(app_t *app, void *stream_ctx,
                                 platform_read_fn read_cb,
                                 platform_write_fn write_cb) {
    char recv_buf[RECV_BUFFER_SIZE];
    size_t total_received = 0;

    /* Per-request Arena memory */
    uint8_t arena_buffer[ARENA_CAPACITY];
    arena_t arena = {
        .buffer = arena_buffer,
        .capacity = ARENA_CAPACITY,
        .offset = 0
    };

    /* Read raw bytes through stream adapter */
    int bytes_read = read_cb(stream_ctx, recv_buf, sizeof(recv_buf) - 1);
    if (bytes_read <= 0) {
        return;
    }
    total_received = (size_t)bytes_read;
    recv_buf[total_received] = '\0';

    /* Parse HTTP/1.1 request (Zero-Copy) */
    http_raw_request_t raw_req;
    size_t bytes_consumed = 0;
    http_parse_result_t parse_res = http_parse_request(recv_buf, total_received, &bytes_consumed, &raw_req);

    if (parse_res == PARSE_OK) {
        http_req_t req;
        http_res_t res;
        req_init(&req, &raw_req, &arena);
        res_init(&res, &arena);

        /* Dispatch through Express-style onion pipeline */
        app_dispatch(app, &req, &res);

        /* Serialize response into wire format */
        char send_buf[RECV_BUFFER_SIZE];
        size_t send_len = res_serialize(&res, send_buf, sizeof(send_buf));
        if (send_len > 0) {
            write_cb(stream_ctx, send_buf, send_len);
        }
    } else {
        /* Fallback 400 Bad Request */
        const char *bad_req = "HTTP/1.1 400 Bad Request\r\nConnection: close\r\nContent-Length: 15\r\n\r\n400 Bad Request";
        write_cb(stream_ctx, bad_req, strlen(bad_req));
    }
}

