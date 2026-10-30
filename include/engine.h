#ifndef CSERVER_ENGINE_H
#define CSERVER_ENGINE_H

#include "server.h"
#include <stdbool.h>

/**
 * Server Engine Orchestrator
 * Controls server lifecycle, configuration, graceful startup, and shutdown.
 */

typedef enum {
    ENGINE_DRIVER_AUTO = 0,
    ENGINE_DRIVER_WIN32,
    ENGINE_DRIVER_POSIX,
    ENGINE_DRIVER_WASI_STREAM,
    ENGINE_DRIVER_WASI_MEMORY
} engine_driver_t;

typedef struct {
    int port;
    const char *host;
    int backlog;
    size_t arena_size;
    engine_driver_t driver;
    bool enable_logging;
} engine_config_t;

typedef struct engine {
    app_t *app;
    engine_config_t config;
    bool is_running;
} engine_t;

/* Default production-ready configuration */
engine_config_t engine_default_config(void);

/* Lifecycle */
engine_t *engine_create(app_t *app, const engine_config_t *config);
int engine_start(engine_t *engine);
void engine_stop(engine_t *engine);
void engine_destroy(engine_t *engine);

#endif /* CSERVER_ENGINE_H */

