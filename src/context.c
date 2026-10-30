#include "../include/context.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <strings.h>

/* Arena memory management */
void *arena_alloc(arena_t *arena, size_t size) {
    if (!arena || !arena->buffer) return NULL;
    /* 8-byte alignment */
    size = (size + 7) & ~7;
    if (arena->offset + size > arena->capacity) {
        return NULL; /* Out of arena space */
    }
    void *ptr = &arena->buffer[arena->offset];
    arena->offset += size;
    return ptr;
}

void arena_reset(arena_t *arena) {
    if (arena) {
        arena->offset = 0;
    }
}

void req_init(http_req_t *req, const http_raw_request_t *raw, arena_t *arena) {
    memset(req, 0, sizeof(*req));
    req->method = raw->method;
    req->path = raw->path;
    req->query = raw->query;
    req->body = raw->body;
    req->header_count = raw->header_count;
    req->arena = arena;
    for (size_t i = 0; i < raw->header_count; ++i) {
        req->headers[i] = raw->headers[i];
    }
}

void res_init(http_res_t *res, arena_t *arena) {
    memset(res, 0, sizeof(*res));
    res->status_code = 200;
    res->arena = arena;
    res->is_sent = false;
}

const char *req_get_header(http_req_t *req, const char *key) {
    size_t klen = strlen(key);
    for (size_t i = 0; i < req->header_count; ++i) {
        if (req->headers[i].key.len == klen &&
            strncasecmp(req->headers[i].key.ptr, key, klen) == 0) {
            /* Create a null-terminated copy in arena for convenience */
            char *copy = (char *)arena_alloc(req->arena, req->headers[i].value.len + 1);
            if (copy) {
                memcpy(copy, req->headers[i].value.ptr, req->headers[i].value.len);
                copy[req->headers[i].value.len] = '\0';
                return copy;
            }
            return req->headers[i].value.ptr;
        }
    }
    return NULL;
}

const char *req_get_param(http_req_t *req, const char *key) {
    context_node_t *curr = req->params;
    while (curr) {
        if (strcmp(curr->key, key) == 0) {
            return (const char *)curr->value;
        }
        curr = curr->next;
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

const char *http_status_text(int code) {
    switch (code) {
        case 200: return "OK";
        case 201: return "Created";
        case 204: return "No Content";
        case 301: return "Moved Permanently";
        case 302: return "Found";
        case 304: return "Not Modified";
        case 400: return "Bad Request";
        case 401: return "Unauthorized";
        case 403: return "Forbidden";
        case 404: return "Not Found";
        case 405: return "Method Not Allowed";
        case 500: return "Internal Server Error";
        case 502: return "Bad Gateway";
        case 503: return "Service Unavailable";
        default:  return "Unknown";
    }
}

size_t res_serialize(const http_res_t *res, char *out_buf, size_t max_len) {
    if (!out_buf || max_len == 0) return 0;

    int written = snprintf(out_buf, max_len, "HTTP/1.1 %d %s\r\n", 
                           res->status_code, http_status_text(res->status_code));
    if (written < 0 || (size_t)written >= max_len) return 0;

    size_t offset = (size_t)written;

    /* Check if Content-Length, Content-Type, or Connection were set */
    bool has_cl = false;
    bool has_ct = false;
    bool has_conn = false;
    for (size_t i = 0; i < res->header_count; ++i) {
        if (strncasecmp(res->headers[i].key.ptr, "Content-Length", res->headers[i].key.len) == 0) {
            has_cl = true;
        }
        if (strncasecmp(res->headers[i].key.ptr, "Content-Type", res->headers[i].key.len) == 0) {
            has_ct = true;
        }
        if (strncasecmp(res->headers[i].key.ptr, "Connection", res->headers[i].key.len) == 0) {
            has_conn = true;
        }
    }

    if (!has_conn) {
        const char *default_conn = "Connection: close\r\n";
        size_t conn_len = strlen(default_conn);
        if (offset + conn_len < max_len) {
            memcpy(out_buf + offset, default_conn, conn_len);
            offset += conn_len;
        }
    }

    if (!has_cl) {
        written = snprintf(out_buf + offset, max_len - offset, "Content-Length: %u\r\n", (unsigned int)res->body_len);
        if (written > 0 && offset + (size_t)written < max_len) {
            offset += (size_t)written;
        }
    }

    if (!has_ct) {
        const char *default_ct = "Content-Type: text/plain\r\n";
        size_t ct_len = strlen(default_ct);
        if (offset + ct_len < max_len) {
            memcpy(out_buf + offset, default_ct, ct_len);
            offset += ct_len;
        }
    }

    /* Write user headers */
    for (size_t i = 0; i < res->header_count; ++i) {
        size_t needed = res->headers[i].key.len + 2 + res->headers[i].value.len + 2;
        if (offset + needed < max_len) {
            memcpy(out_buf + offset, res->headers[i].key.ptr, res->headers[i].key.len);
            offset += res->headers[i].key.len;
            memcpy(out_buf + offset, ": ", 2);
            offset += 2;
            memcpy(out_buf + offset, res->headers[i].value.ptr, res->headers[i].value.len);
            offset += res->headers[i].value.len;
            memcpy(out_buf + offset, "\r\n", 2);
            offset += 2;
        }
    }

    /* Final CRLF before body */
    if (offset + 2 < max_len) {
        memcpy(out_buf + offset, "\r\n", 2);
        offset += 2;
    }

    /* Write body */
    if (res->body_len > 0 && res->body) {
        if (offset + res->body_len < max_len) {
            memcpy(out_buf + offset, res->body, res->body_len);
            offset += res->body_len;
        }
    }

    return offset;
}
