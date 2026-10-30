#ifndef CSERVER_SERVER_H
#define CSERVER_SERVER_H

#include "context.h"
#include "http_proto.h"

/**
 * Express-like Application Pipeline
 * Handlers use the (req, res, next) signature for an onion/chained architecture.
 */

struct app;
typedef struct app app_t;

/* Next callback signature */
typedef void (*http_next_fn)(void *pipeline_state);

/* Middleware / Handler signature */
typedef void (*http_handler_fn)(http_req_t *req, http_res_t *res, http_next_fn next, void *pipeline_state);

typedef struct middleware_entry {
    http_method_t method;      /* HTTP_METHOD_UNKNOWN matches any method */
    const char *path_prefix;   /* Route prefix (or NULL/"/" for global middleware) */
    http_handler_fn handler;
    struct middleware_entry *next;
} middleware_entry_t;

struct app {
    middleware_entry_t *middleware_head;
    middleware_entry_t *middleware_tail;
};

/* Application lifecycle */
app_t *app_create(void);
void app_destroy(app_t *app);

/* Express-like registration API */
void app_use(app_t *app, const char *path, http_handler_fn handler);
void app_get(app_t *app, const char *path, http_handler_fn handler);
void app_post(app_t *app, const char *path, http_handler_fn handler);
void app_put(app_t *app, const char *path, http_handler_fn handler);
void app_delete(app_t *app, const char *path, http_handler_fn handler);

/* Dispatch an incoming request through the registered middleware/routes */
void app_dispatch(app_t *app, http_req_t *req, http_res_t *res);

/* Platform-dependent socket listener starter */
int app_listen(app_t *app, int port);

#endif /* CSERVER_SERVER_H */

