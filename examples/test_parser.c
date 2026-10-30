#include "cserver.h"
#include <stdio.h>
#include <string.h>
#include <assert.h>

int main(void) {
    printf("=== Testing Sans-I/O HTTP Parser ===\n\n");

    /* Test 1: Full GET request with query string */
    const char *raw_get =
        "GET /api/users?page=2&limit=20 HTTP/1.1\r\n"
        "Host: localhost:8080\r\n"
        "User-Agent: curl/8.4.0\r\n"
        "Accept: application/json\r\n"
        "\r\n";

    http_raw_request_t req;
    size_t consumed = 0;
    http_parse_result_t res = http_parse_request(raw_get, strlen(raw_get), &consumed, &req);

    assert(res == PARSE_OK);
    assert(req.method == HTTP_METHOD_GET);
    assert(http_sv_equals(req.path, "/api/users"));
    assert(http_sv_equals(req.query, "page=2&limit=20"));
    assert(req.header_count == 3);

    const http_sv_t *host_val = http_get_header(&req, "host");
    assert(host_val != NULL);
    assert(http_sv_equals(*host_val, "localhost:8080"));

    printf("[PASS] Test 1: GET Request parsed correctly\n");
    printf("       Method: %s\n", http_method_to_string(req.method));
    printf("       Path:   %.*s\n", (int)req.path.len, req.path.ptr);
    printf("       Query:  %.*s\n", (int)req.query.len, req.query.ptr);
    printf("       Host:   %.*s\n", (int)host_val->len, host_val->ptr);

    /* Test 2: Incomplete chunk (streaming TCP simulation) */
    const char *partial = "POST /submit HTTP/1.1\r\nHost: loc";
    res = http_parse_request(partial, strlen(partial), &consumed, &req);
    assert(res == PARSE_NEED_MORE_DATA);
    printf("[PASS] Test 2: Incomplete packet correctly triggers PARSE_NEED_MORE_DATA\n");

    /* Test 3: POST request with Content-Length and body */
    const char *raw_post =
        "POST /api/login HTTP/1.1\r\n"
        "Host: localhost:8080\r\n"
        "Content-Type: application/json\r\n"
        "Content-Length: 27\r\n"
        "\r\n"
        "{\"user\":\"neo\",\"pass\":\"123\"}";

    res = http_parse_request(raw_post, strlen(raw_post), &consumed, &req);
    assert(res == PARSE_OK);
    assert(req.method == HTTP_METHOD_POST);
    assert(http_sv_equals(req.path, "/api/login"));
    assert(req.body.len == 27);
    assert(http_sv_equals(req.body, "{\"user\":\"neo\",\"pass\":\"123\"}"));
    assert(consumed == strlen(raw_post));

    printf("[PASS] Test 3: POST with body parsed correctly\n");
    printf("       Body (len=%u): %.*s\n", (unsigned int)req.body.len, (int)req.body.len, req.body.ptr);

    printf("\nAll parser tests passed with 0 allocations!\n");
    return 0;
}

