#ifndef CSERVER_CONTEXT_H
#define CSERVER_CONTEXT_H

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include "http_proto.h"

/**
 * Context, Request, and Response Abstraction
 * Uses a linear Arena Allocator for zero heap fragmentation during request lifecycles.
 */

/* Minimal linear scratch allocator per request */
typedef struct {
    uint8_t *buffer;
    size_t capacity;
    size_t offset;
} arena_t;

void *arena_alloc(arena_t *arena, size_t size);
void arena_reset(arena_t *arena);

/* Key-Value context entry for template variables / locals */
typedef struct context_node {
    const char *key;
    void *value;
    struct context_node *next;
} context_node_t;

/* Request representation */
typedef struct http_req {
    http_method_t method;
    http_sv_t path;
    http_sv_t query;
    http_sv_t body;
    http_header_t headers[MAX_HTTP_HEADERS];
    size_t header_count;
    
    /* Request-scoped memory arena */
    arena_t *arena;
    
    /* Route params if matched (e.g., :id) */
    context_node_t *params;
} http_req_t;

/* Response representation */
typedef struct http_res {
    int status_code;
    http_header_t headers[MAX_HTTP_HEADERS];
    size_t header_count;
    
    /* Response body output buffer */
    char *body;
    size_t body_len;
    size_t body_capacity;
    
    bool is_sent;
    arena_t *arena;
} http_res_t;

/* Context helper functions */
void req_init(http_req_t *req, const http_raw_request_t *raw, arena_t *arena);
void res_init(http_res_t *res, arena_t *arena);

const char *req_get_header(http_req_t *req, const char *key);
const char *req_get_param(http_req_t *req, const char *key);

void res_status(http_res_t *res, int code);
void res_set_header(http_res_t *res, const char *key, const char *val);
void res_send(http_res_t *res, const char *body);
void res_json(http_res_t *res, const char *json_string);

/* Response serialization to HTTP/1.1 wire format */
const char *http_status_text(int code);
size_t res_serialize(const http_res_t *res, char *out_buf, size_t max_len);

#endif /* CSERVER_CONTEXT_H */

