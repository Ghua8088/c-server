#include "../include/http_proto.h"
#include <string.h>
#include <ctype.h>
#include <stdlib.h>

const char *http_method_to_string(http_method_t method) {
    switch (method) {
        case HTTP_METHOD_GET:     return "GET";
        case HTTP_METHOD_POST:    return "POST";
        case HTTP_METHOD_PUT:     return "PUT";
        case HTTP_METHOD_DELETE:  return "DELETE";
        case HTTP_METHOD_HEAD:    return "HEAD";
        case HTTP_METHOD_OPTIONS: return "OPTIONS";
        default:                  return "UNKNOWN";
    }
}

http_method_t http_string_to_method(const char *str, size_t len) {
    if (len == 3 && memcmp(str, "GET", 3) == 0) return HTTP_METHOD_GET;
    if (len == 4 && memcmp(str, "POST", 4) == 0) return HTTP_METHOD_POST;
    if (len == 3 && memcmp(str, "PUT", 3) == 0) return HTTP_METHOD_PUT;
    if (len == 6 && memcmp(str, "DELETE", 6) == 0) return HTTP_METHOD_DELETE;
    if (len == 4 && memcmp(str, "HEAD", 4) == 0) return HTTP_METHOD_HEAD;
    if (len == 7 && memcmp(str, "OPTIONS", 7) == 0) return HTTP_METHOD_OPTIONS;
    return HTTP_METHOD_UNKNOWN;
}

bool http_sv_equals(http_sv_t sv, const char *cstr) {
    size_t clen = strlen(cstr);
    return (sv.len == clen) && (memcmp(sv.ptr, cstr, clen) == 0);
}

bool http_sv_case_equals(http_sv_t sv, const char *cstr) {
    size_t clen = strlen(cstr);
    if (sv.len != clen) return false;
    for (size_t i = 0; i < clen; ++i) {
        if (tolower((unsigned char)sv.ptr[i]) != tolower((unsigned char)cstr[i])) {
            return false;
        }
    }
    return true;
}

const http_sv_t *http_get_header(const http_raw_request_t *req, const char *key) {
    for (size_t i = 0; i < req->header_count; ++i) {
        if (http_sv_case_equals(req->headers[i].key, key)) {
            return &req->headers[i].value;
        }
    }
    return NULL;
}

static const char *find_crlf(const char *buf, size_t len) {
    for (size_t i = 0; i + 1 < len; ++i) {
        if (buf[i] == '\r' && buf[i + 1] == '\n') {
            return &buf[i];
        }
    }
    return NULL;
}

static const char *find_header_end(const char *buf, size_t len, size_t *body_offset) {
    for (size_t i = 0; i + 3 < len; ++i) {
        if (buf[i] == '\r' && buf[i + 1] == '\n' && buf[i + 2] == '\r' && buf[i + 3] == '\n') {
            *body_offset = i + 4;
            return &buf[i + 2]; /* Point right after last header's CRLF */
        }
    }
    /* Fallback for relaxed clients using \n\n */
    for (size_t i = 0; i + 1 < len; ++i) {
        if (buf[i] == '\n' && buf[i + 1] == '\n') {
            *body_offset = i + 2;
            return &buf[i + 1];
        }
    }
    return NULL;
}

http_parse_result_t http_parse_request(const char *buf, size_t len, size_t *bytes_consumed, http_raw_request_t *out_req) {
    if (!buf || len == 0 || !out_req) {
        return PARSE_NEED_MORE_DATA;
    }

    /* 1. Find end of headers */
    size_t body_offset = 0;
    const char *headers_end = find_header_end(buf, len, &body_offset);
    if (!headers_end) {
        return PARSE_NEED_MORE_DATA; /* Headers not fully received yet */
    }

    memset(out_req, 0, sizeof(*out_req));

    /* 2. Parse Request Line: METHOD URI HTTP/X.Y\r\n */
    const char *line_end = find_crlf(buf, (size_t)(headers_end - buf));
    if (!line_end) {
        return PARSE_ERROR_INVALID;
    }

    const char *p = buf;
    const char *method_start = p;
    while (p < line_end && *p != ' ') p++;
    if (p == line_end) return PARSE_ERROR_INVALID;

    out_req->method = http_string_to_method(method_start, (size_t)(p - method_start));
    if (out_req->method == HTTP_METHOD_UNKNOWN) return PARSE_ERROR_INVALID;

    /* Skip spaces before URI */
    while (p < line_end && *p == ' ') p++;
    const char *uri_start = p;
    while (p < line_end && *p != ' ') p++;
    if (p == line_end) return PARSE_ERROR_INVALID;

    out_req->uri.ptr = uri_start;
    out_req->uri.len = (size_t)(p - uri_start);

    /* Split URI into Path and Query */
    const char *qmark = memchr(uri_start, '?', out_req->uri.len);
    if (qmark) {
        out_req->path.ptr = uri_start;
        out_req->path.len = (size_t)(qmark - uri_start);
        out_req->query.ptr = qmark + 1;
        out_req->query.len = (size_t)((uri_start + out_req->uri.len) - (qmark + 1));
    } else {
        out_req->path = out_req->uri;
        out_req->query.ptr = NULL;
        out_req->query.len = 0;
    }

    /* Skip spaces before Protocol Version */
    while (p < line_end && *p == ' ') p++;
    if ((size_t)(line_end - p) >= 8 && memcmp(p, "HTTP/1.", 7) == 0) {
        out_req->version_major = 1;
        out_req->version_minor = p[7] - '0';
    } else {
        return PARSE_ERROR_INVALID;
    }

    /* Advance past Request Line CRLF */
    p = line_end + 2;

    /* 3. Parse Header Lines */
    while (p < headers_end) {
        const char *next_crlf = find_crlf(p, (size_t)(headers_end - p));
        if (!next_crlf) break;

        const char *colon = memchr(p, ':', (size_t)(next_crlf - p));
        if (colon && out_req->header_count < MAX_HTTP_HEADERS) {
            http_header_t *h = &out_req->headers[out_req->header_count++];
            h->key.ptr = p;
            h->key.len = (size_t)(colon - p);

            /* Trim leading space on value */
            const char *val_start = colon + 1;
            while (val_start < next_crlf && (*val_start == ' ' || *val_start == '\t')) {
                val_start++;
            }
            /* Trim trailing space on value */
            const char *val_end = next_crlf;
            while (val_end > val_start && (*(val_end - 1) == ' ' || *(val_end - 1) == '\t')) {
                val_end--;
            }

            h->value.ptr = val_start;
            h->value.len = (size_t)(val_end - val_start);
        }

        p = next_crlf + 2;
    }

    /* 4. Determine Keep-Alive */
    const http_sv_t *conn_hdr = http_get_header(out_req, "Connection");
    if (out_req->version_minor == 1) {
        out_req->keep_alive = !(conn_hdr && http_sv_case_equals(*conn_hdr, "close"));
    } else {
        out_req->keep_alive = (conn_hdr && http_sv_case_equals(*conn_hdr, "keep-alive"));
    }

    /* 5. Determine Body & Content-Length */
    size_t content_length = 0;
    const http_sv_t *cl_hdr = http_get_header(out_req, "Content-Length");
    if (cl_hdr && cl_hdr->len > 0) {
        content_length = (size_t)strtoul(cl_hdr->ptr, NULL, 10);
    }

    /* Check if the entire body is in the buffer */
    if (len < body_offset + content_length) {
        return PARSE_NEED_MORE_DATA;
    }

    if (content_length > 0) {
        out_req->body.ptr = buf + body_offset;
        out_req->body.len = content_length;
    } else {
        out_req->body.ptr = NULL;
        out_req->body.len = 0;
    }

    if (bytes_consumed) {
        *bytes_consumed = body_offset + content_length;
    }

    return PARSE_OK;
}
