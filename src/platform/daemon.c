#include "../../include/server.h"

#if defined(_WIN32)
    int platform_listen_win32(app_t *app, int port);
#elif defined(__wasi__)
    int platform_listen_wasi(app_t *app, int port);
#else
    int platform_listen_posix(app_t *app, int port);
#endif

int app_listen(app_t *app, int port) {
#if defined(_WIN32)
    return platform_listen_win32(app, port);
#elif defined(__wasi__)
    return platform_listen_wasi(app, port);
#else
    return platform_listen_posix(app, port);
#endif
}
