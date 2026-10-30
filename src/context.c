#include "../include/context.h"
#include <stdlib.h>
#include <string.h>

/* Arena memory management */
void *arena_alloc(arena_t *arena, size_t size) {
    /* Align to 8 bytes */
    size = (size + 7) & ~7;
    if (arena->offset + size > arena->capacity) {
        return NULL; /* Out of arena space */
    }
    void *ptr = &arena->buffer[arena->offset];
    arena->offset += size;
    return ptr;
}

void arena_reset(arena_t *arena) {
    arena->offset = 0;
}

const char *req_get_header(http_req_t *req, const char *key) {
    size_t klen = strlen(key);
    for (size_t i = 0; i < req->header_count; ++i) {
        if (req->headers[i].key.len == klen &&
            strncasecmp(req->headers[i].key.ptr, key, klen) == 0) {
            return req->headers[i].value.ptr;
        }
    }
    return NULL;
}

void res_status(http_res_t *res, int code) {
    res->status_code = code;
}

void res_set_header(http_res_t *res, const char *key, const char *val) {
    if (res->header_count >= MAX_HTTP_HEADERS) return;
    res->headers[res->header_count].key.ptr = key;
    res->headers[res->header_count].key.len = strlen(key);
    res->headers[res->header_count].value.ptr = val;
    res->headers[res->header_count].value.len = strlen(val);
    res->header_count++;
}

void res_send(http_res_t *res, const char *body) {
    size_t len = strlen(body);
    res->body = (char *)arena_alloc(res->arena, len + 1);
    if (res->body) {
        memcpy(res->body, body, len + 1);
        res->body_len = len;
    }
    res->is_sent = true;
}

void res_json(http_res_t *res, const char *json_string) {
    res_set_header(res, "Content-Type", "application/json");
    res_send(res, json_string);
}

