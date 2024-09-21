#include <limits.h>
#include <unistd.h>
#include <wayland-client-core.h>

#include "output/shared/graphical.h"

#include "gen/xdg-shell-client-protocol.h"

#include "xdg.h"
#include "main.h"
#ifdef EGL
    #include "egl.h"
#endif
#ifdef SHM
    #include "shm.h"
#endif
#ifdef CAIRO
    #include "cairo.h"
#endif

static struct xdg_surface *xavaXDGSurface;
static struct xdg_toplevel *xavaXDGToplevel;

struct xdg_wm_base *xavaXDGWMBase;

static void xdg_wm_base_ping(void *data, struct xdg_wm_base *xdg_wm_base,
        uint32_t serial) {
    UNUSED(data);
    xdg_wm_base_pong(xdg_wm_base, serial);
}

const struct xdg_wm_base_listener xdg_wm_base_listener = {
    .ping = xdg_wm_base_ping,
};

static void xdg_toplevel_handle_configure(void *data,
        struct xdg_toplevel *xdg_toplevel, int32_t w, int32_t h,
        struct wl_array *states) {
    UNUSED(xdg_toplevel);
    UNUSED(states);

    struct waydata       *wd   = data;
    XAVA   *xava = wd->hand;
    XAVA_CONFIG *conf = &xava->conf;

    if(w == 0 && h == 0) return;

    if(conf->w != (uint32_t)w && conf->h != (uint32_t)h) {
        // fixme when i get proper monitor support on XDG
        calculate_win_geo(xava, w, h);

        #ifdef EGL
            waylandEGLWindowResize(wd, w, h);
        #endif

        #ifdef SHM
            reallocSHM(wd);
        #endif

        #ifdef CAIRO
            xava_output_wayland_cairo_resize(wd);
        #endif

        pushXAVAEventStack(wd->events, XAVA_REDRAW);
        pushXAVAEventStack(wd->events, XAVA_RESIZE);
    }

    #ifdef SHM
        update_frame(wd);
    #endif
}

static void xdg_toplevel_handle_close(void *data,
        struct xdg_toplevel *xdg_toplevel) {
    struct waydata           *wd   = data;

    UNUSED(xdg_toplevel);

    pushXAVAEventStack(wd->events, XAVA_QUIT);
}

struct xdg_toplevel_listener xdg_toplevel_listener = {
    .configure = xdg_toplevel_handle_configure,
    .close = xdg_toplevel_handle_close,
};

static void xdg_surface_configure(void *data, struct xdg_surface *xdg_surface,
        uint32_t serial) {
    struct waydata *wd = data;

    // confirm that you exist to the compositor
    xdg_surface_ack_configure(xdg_surface, serial);

    wl_surface_commit(wd->surface);
}

const struct xdg_surface_listener xdg_surface_listener = {
    .configure = xdg_surface_configure,
};

void xdg_init(struct waydata *wd) {
    // create window, or "surface" in waland terms
    xavaXDGSurface = xdg_wm_base_get_xdg_surface(xavaXDGWMBase, wd->surface);

    // for those unaware, the compositor baby sits everything that you
    // make, thus it needs a function through which the compositor
    // will manage your application
    xdg_surface_add_listener(xavaXDGSurface, &xdg_surface_listener, wd);

    xavaXDGToplevel = xdg_surface_get_toplevel(xavaXDGSurface);
    xdg_toplevel_set_title(xavaXDGToplevel, "XAVA");
    xdg_toplevel_add_listener(xavaXDGToplevel, &xdg_toplevel_listener, wd);
}

void xdg_cleanup() {
    xdg_toplevel_destroy(xavaXDGToplevel);
    xdg_surface_destroy(xavaXDGSurface);
}
