#include "../include/server.h"
#include <stdlib.h>
#include <string.h>

app_t *app_create(void) {
    app_t *app = (app_t *)calloc(1, sizeof(app_t));
    return app;
}

void app_destroy(app_t *app) {
    if (!app) return;
    middleware_entry_t *curr = app->middleware_head;
    while (curr) {
        middleware_entry_t *next = curr->next;
        free(curr);
        curr = next;
    }
    free(app);
}

static void register_route(app_t *app, http_method_t method, const char *path, http_handler_fn handler) {
    middleware_entry_t *entry = (middleware_entry_t *)calloc(1, sizeof(middleware_entry_t));
    entry->method = method;
    entry->path_prefix = path ? path : "/";
    entry->handler = handler;

    if (!app->middleware_head) {
        app->middleware_head = entry;
        app->middleware_tail = entry;
    } else {
        app->middleware_tail->next = entry;
        app->middleware_tail = entry;
    }
}

void app_use(app_t *app, const char *path, http_handler_fn handler) {
    register_route(app, HTTP_METHOD_UNKNOWN, path, handler);
}

void app_get(app_t *app, const char *path, http_handler_fn handler) {
    register_route(app, HTTP_METHOD_GET, path, handler);
}

void app_post(app_t *app, const char *path, http_handler_fn handler) {
    register_route(app, HTTP_METHOD_POST, path, handler);
}

void app_put(app_t *app, const char *path, http_handler_fn handler) {
    register_route(app, HTTP_METHOD_PUT, path, handler);
}

void app_delete(app_t *app, const char *path, http_handler_fn handler) {
    register_route(app, HTTP_METHOD_DELETE, path, handler);
}

/* Dispatcher pipeline state */
typedef struct {
    middleware_entry_t *current;
    http_req_t *req;
    http_res_t *res;
} pipeline_state_t;

static void pipeline_step(void *state_ptr);

static void pipeline_step(void *state_ptr) {
    pipeline_state_t *state = (pipeline_state_t *)state_ptr;
    while (state->current && !state->res->is_sent) {
        middleware_entry_t *entry = state->current;
        state->current = entry->next;

        /* Method check */
        bool method_matches = (entry->method == HTTP_METHOD_UNKNOWN) || (entry->method == state->req->method);
        if (!method_matches) continue;

        /* Basic path prefix matching */
        if (entry->path_prefix && strcmp(entry->path_prefix, "/") != 0) {
            size_t prefix_len = strlen(entry->path_prefix);
            if (state->req->path.len < prefix_len ||
                strncmp(state->req->path.ptr, entry->path_prefix, prefix_len) != 0) {
                continue;
            }
        }

        /* Invoke handler passing pipeline_step as `next` */
        entry->handler(state->req, state->res, pipeline_step, state);
        return;
    }

    /* Fallback 404 if response was never sent */
    if (!state->res->is_sent) {
        res_status(state->res, 404);
        res_send(state->res, "404 Not Found");
    }
}

void app_dispatch(app_t *app, http_req_t *req, http_res_t *res) {
    pipeline_state_t state = {
        .current = app->middleware_head,
        .req = req,
        .res = res
    };
    pipeline_step(&state);
}

