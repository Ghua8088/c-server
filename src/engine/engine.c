#include "../../include/engine.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

engine_config_t engine_default_config(void) {
    engine_config_t cfg;
    memset(&cfg, 0, sizeof(cfg));
    cfg.port = 8080;
    cfg.host = "0.0.0.0";
    cfg.backlog = 128;
    cfg.arena_size = 65536;
    cfg.driver = ENGINE_DRIVER_AUTO;
    cfg.enable_logging = true;
    return cfg;
}

engine_t *engine_create(app_t *app, const engine_config_t *config) {
    if (!app) return NULL;
    engine_t *eng = (engine_t *)calloc(1, sizeof(engine_t));
    if (!eng) return NULL;

    eng->app = app;
    if (config) {
        eng->config = *config;
    } else {
        eng->config = engine_default_config();
    }

    return eng;
}

int engine_start(engine_t *engine) {
    if (!engine || !engine->app) return -1;
    engine->is_running = true;

    if (engine->config.enable_logging) {
        printf("[Engine] Starting server engine...\n");
        printf("[Engine] Target Host: %s, Port: %d\n", engine->config.host, engine->config.port);
        printf("[Engine] Request Scratch Arena: %u KB\n", (unsigned int)(engine->config.arena_size / 1024));
        fflush(stdout);
    }

    /* Start via platform driver */
    int res = app_listen(engine->app, engine->config.port);
    engine->is_running = false;
    return res;
}

void engine_stop(engine_t *engine) {
    if (engine) {
        engine->is_running = false;
    }
}

void engine_destroy(engine_t *engine) {
    if (engine) {
        free(engine);
    }
}
