#ifndef CSERVER_HTTP_PROTO_H
#define CSERVER_HTTP_PROTO_H

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

/**
 * Sans-I/O HTTP/1.1 Protocol Definitions
 * Zero OS dependencies — 100% portable to WASM / WASI.
 */

typedef enum {
    HTTP_METHOD_UNKNOWN = 0,
    HTTP_METHOD_GET,
    HTTP_METHOD_POST,
    HTTP_METHOD_PUT,
    HTTP_METHOD_DELETE,
    HTTP_METHOD_HEAD,
    HTTP_METHOD_OPTIONS
} http_method_t;

typedef struct {
    const char *ptr;
    size_t len;
} http_sv_t; /* String View (zero-copy slice) */

typedef struct {
    http_sv_t key;
    http_sv_t value;
} http_header_t;

#define MAX_HTTP_HEADERS 32

typedef struct {
    http_method_t method;
    http_sv_t uri;
    http_sv_t path;
    http_sv_t query;
    int version_major;
    int version_minor;
    http_header_t headers[MAX_HTTP_HEADERS];
    size_t header_count;
    http_sv_t body;
    bool keep_alive;
} http_raw_request_t;

typedef enum {
    PARSE_NEED_MORE_DATA,
    PARSE_OK,
    PARSE_ERROR_INVALID
} http_parse_result_t;

/* Parse HTTP/1.1 request from a raw byte buffer (Sans-I/O) */
http_parse_result_t http_parse_request(const char *buf, size_t len, size_t *bytes_consumed, http_raw_request_t *out_req);

const char *http_method_to_string(http_method_t method);
http_method_t http_string_to_method(const char *str, size_t len);

/* String View helpers */
bool http_sv_equals(http_sv_t sv, const char *cstr);
bool http_sv_case_equals(http_sv_t sv, const char *cstr);
const http_sv_t *http_get_header(const http_raw_request_t *req, const char *key);

#endif /* CSERVER_HTTP_PROTO_H */

