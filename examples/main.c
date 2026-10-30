#include "cserver.h"
#include <stdio.h>

/* Custom Logger Middleware */
void logger_middleware(http_req_t *req, http_res_t *res, http_next_fn next, void *state) {
    (void)res;
    printf("[LOG] %s request to %.*s\n", 
           http_method_to_string(req->method), 
           (int)req->path.len, req->path.ptr);
    
    /* Call next middleware/handler in chain */
    next(state);
}

/* Route Handlers */
void handle_hello(http_req_t *req, http_res_t *res, http_next_fn next, void *state) {
    (void)req; (void)next; (void)state;
    res_status(res, 200);
    res_send(res, "Hello, Minimal WASM-ready C HTTP Server!");
}

void handle_api_status(http_req_t *req, http_res_t *res, http_next_fn next, void *state) {
    (void)req; (void)next; (void)state;
    res_status(res, 200);
    res_json(res, "{\"status\": \"ok\", \"engine\": \"cserver-http1.1\"}");
}

int main(void) {
    app_t *app = app_create();

    /* 1. Global Middleware */
    app_use(app, NULL, logger_middleware);

    /* 2. Routes */
    app_get(app, "/", handle_hello);
    app_get(app, "/api/status", handle_api_status);

    /* 3. Start Listener */
    app_listen(app, 8080);

    app_destroy(app);
    return 0;
}
