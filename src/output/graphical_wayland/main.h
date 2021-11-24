#ifndef __WAYLAND_MAIN_H
#define __WAYLAND_MAIN_H

#ifdef EGL
    #include <stdint.h>
    #include "../shared/gl/egl.h"
#endif

#include <wayland-client.h>

#include "../../shared.h"

struct waydata {
    struct wl_surface    *surface;
    struct wl_display    *display;
    struct wl_compositor *compositor;
    struct XAVA_HANDLE   *hand;
    XG_EVENT_STACK       *events;
    struct wl_list       outputs;
    #ifdef EGL
        struct _escontext ESContext;
    #endif
};


extern const struct wl_callback_listener wl_surface_frame_listener;

extern char* monitorName;

EXP_FUNC void xavaOutputClear(struct XAVA_HANDLE *hand);

#endif
