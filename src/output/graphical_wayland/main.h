#ifndef __WAYLAND_MAIN_H
#define __WAYLAND_MAIN_H

#ifdef EGL
    #include <stdint.h>
    #include "output/shared/gl/egl.h"
#endif

#ifdef CAIRO
    #include <cairo.h>
    #include "output/shared/cairo/main.h"
#endif

#include <wayland-client.h>

#include "shared.h"

struct waydata {
    struct wl_surface    *surface;
    struct wl_display    *display;
    struct wl_compositor *compositor;
    XAVA   *hand;
    XG_EVENT_STACK       *events;
    struct wl_list       outputs;
    #ifdef EGL
        struct _escontext ESContext;
    #endif
    #ifdef SHM
        struct wayland_shm {
            struct dimenisions {
                uint32_t w;
                uint32_t h;
                uint32_t stride;
            } dim;
            struct wl_shm *ref;
            uint32_t      max_size;
            uint8_t       *buffer;
            int32_t       fd;
            bool          fb_unsafe;
        } shm;
    #endif
    #ifdef CAIRO
        cairo_surface_t   *cairo_surface;
        xava_cairo_handle *cairo_handle;
    #endif
};


extern const struct wl_callback_listener wl_surface_frame_listener;

extern char* monitorName;

EXP_FUNC void xavaOutputClear(XAVA *hand);

#endif
