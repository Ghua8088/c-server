#include "../../include/cserver.h"
#include <string.h>

#if defined(__GNUC__) || defined(__clang__)
    #define WASM_EXPORT __attribute__((visibility("default")))
#else
    #define WASM_EXPORT
#endif

static app_t *g_wasm_app = NULL;

/* Demo handlers for WASM environment */
static void wasm_handle_root(http_req_t *req, http_res_t *res, http_next_fn next, void *state) {
    (void)req; (void)next; (void)state;
    res_status(res, 200);
    res_send(res, "Hello from C WASM running inside Bun / Cloudflare!");
}

static void wasm_handle_status(http_req_t *req, http_res_t *res, http_next_fn next, void *state) {
    (void)req; (void)next; (void)state;
    res_status(res, 200);
    res_json(res, "{\"runtime\": \"bun\", \"core\": \"c-server-wasm\", \"status\": \"online\"}");
}

WASM_EXPORT
void wasm_init_engine(void) {
    if (g_wasm_app) return;
    g_wasm_app = app_create();
    app_get(g_wasm_app, "/", wasm_handle_root);
    app_get(g_wasm_app, "/status", wasm_handle_status);
}

WASM_EXPORT
int wasm_handle_http(const char *in_buf, int in_len, char *out_buf, int max_out_len) {
    if (!g_wasm_app) {
        wasm_init_engine();
    }
    if (in_len <= 0 || !in_buf || !out_buf || max_out_len <= 0) {
        return 0;
    }

    /* Per-request Arena */
    uint8_t arena_buffer[65536];
    arena_t arena = {
        .buffer = arena_buffer,
        .capacity = sizeof(arena_buffer),
        .offset = 0
    };

    http_raw_request_t raw_req;
    size_t consumed = 0;
    http_parse_result_t parse_res = http_parse_request(in_buf, (size_t)in_len, &consumed, &raw_req);

    if (parse_res == PARSE_OK) {
        http_req_t req;
        http_res_t res;
        req_init(&req, &raw_req, &arena);
        res_init(&res, &arena);

        app_dispatch(g_wasm_app, &req, &res);
        return (int)res_serialize(&res, out_buf, (size_t)max_out_len);
    } else {
        const char *bad = "HTTP/1.1 400 Bad Request\r\nConnection: close\r\nContent-Length: 15\r\n\r\n400 Bad Request";
        size_t blen = strlen(bad);
        if ((int)blen < max_out_len) {
            memcpy(out_buf, bad, blen);
            return (int)blen;
        }
        return 0;
    }
}

