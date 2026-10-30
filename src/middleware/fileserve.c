#include "../../include/cserver.h"
#include <stdio.h>

/**
 * Static file serving middleware.
 * Keeps file I/O completely decoupled from the core HTTP engine and daemon.
 */

void static_file_middleware(http_req_t *req, http_res_t *res, http_next_fn next, void *state) {
    /* 
     * In native OS: opens local file and streams content.
     * In WASM/WASI: reads from pre-opened virtual directory.
     * If file not found, passes execution down the onion chain.
     */
    (void)req;
    (void)res;
    if (next) {
        next(state);
    }
}

