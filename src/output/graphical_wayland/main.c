#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <wayland-client-core.h>
#include <wayland-client-protocol.h>
#include <unistd.h>
#include <sys/syscall.h>

#include "output/shared/graphical.h"
#include "shared.h"

#include "main.h"
#include "wl_output.h"
#include "registry.h"
#include "zwlr.h"
#include "xdg.h"
#ifdef EGL
    #include <wayland-egl.h>
    #include <wayland-egl-core.h>
    #include "egl.h"
    #include "output/shared/gl/egl.h"
#endif
#ifdef SHM
    #include "shm.h"
#endif
#ifdef CAIRO
    #include "cairo.h"
    #include "output/shared/cairo/main.h"
#endif

/* Globals */
struct waydata wd;

static bool backgroundLayer;
       char* monitorName;

uint32_t fgcol,bgcol;

EXP_FUNC void xavaOutputCleanup(void *v) {
    UNUSED(v);

    #ifdef EGL
        EGLCleanup(wd.hand, &wd.ESContext);
        wl_egl_window_destroy((struct wl_egl_window*)
                wd.ESContext.native_window);
    #endif
    #ifdef CAIRO
        __internal_xava_output_cairo_cleanup(wd.cairo_handle);
        closeSHM(&wd);
    #endif

    if(backgroundLayer) {
        zwlr_cleanup(&wd);
    } else {
        xdg_cleanup();
    }
    wl_output_cleanup(&wd);
    wl_surface_destroy(wd.surface);
    wl_compositor_destroy(wd.compositor);
    wl_registry_destroy(xavaWLRegistry);
    wl_display_disconnect(wd.display);
    free(monitorName);
}

EXP_FUNC int xavaInitOutput(XAVA *hand) {
    wd.hand   = hand;
    wd.events = newXAVAEventStack();

    wd.display = wl_display_connect(NULL);
    xavaBailCondition(!wd.display, "Failed to connect to Wayland server");

    // Before the registry shananigans, outputs must be initialized
    wl_output_init(&wd);

    xavaWLRegistry = wl_display_get_registry(wd.display);
    // TODO: Check failure states
    wl_registry_add_listener(xavaWLRegistry, &xava_wl_registry_listener, &wd);
    wl_display_roundtrip(wd.display);
    xavaBailCondition(!wd.compositor, "Your compositor doesn't support wl_compositor, failing...");
    xavaBailCondition(!xavaXDGWMBase, "Your compositor doesn't support xdg_wm_base, failing...");

    if(xavaWLRLayerShell == NULL || xavaXDGOutputManager == NULL) {
        xavaWarn("Your compositor doesn't support some or any of the following:\n"
                "zwlr_layer_shell_v1 and/or zwlr_output_manager_v1\n"
                "This will DISABLE the ability to use the background layer for"
                "safety reasons!");
        backgroundLayer = 0;
    }

    // needed to be done twice for xdg_output to do it's frickin' job
    wl_display_roundtrip(wd.display);

    wd.surface = wl_compositor_create_surface(wd.compositor);

    // The option carries the same functionality here to Wayland as well
    if(backgroundLayer) {
        zwlr_init(&wd);
    } else {
        xdg_init(&wd);
    }

    //wl_surface_set_buffer_scale(xavaWLSurface, 3);

    // process all of this, FINALLY
    wl_surface_commit(wd.surface);

    #ifdef EGL
        // creates everything EGL related
        waylandEGLCreateWindow(&wd);

        xavaBailCondition(EGLCreateContext(wd.hand, &wd.ESContext) == EGL_FALSE,
                "Failed to create EGL context");

        calculate_win_geo(hand, hand->conf.w, hand->conf.h);

        EGLInit(hand);
    #endif
    #ifdef SHM
        // because wl_shm needs the framebuffer size **NOW**
        // we are going to provide it with the default size
        calculate_win_geo(hand, hand->conf.w, hand->conf.h);

        wd.shm.fd = syscall(SYS_memfd_create, "buffer", 0);

        wd.shm.max_size = 0;
        wd.shm.fb_unsafe = false;

        reallocSHM(&wd);
    #endif
    #ifdef CAIRO
        xava_output_wayland_cairo_init(&wd);
    #endif

    #ifdef SHM
        struct wl_callback *cb = wl_surface_frame(wd.surface);
        wl_callback_add_listener(cb, &wl_surface_frame_listener, &wd);
    #endif
    

    return EXIT_SUCCESS;
}

EXP_FUNC void xavaOutputClear(XAVA *hand) {
    #ifdef CAIRO
        UNUSED(hand);
        __internal_xava_output_cairo_clear(wd.cairo_handle);
    #elif defined(EGL)
        EGLClear(hand);
    #else
        UNUSED(hand);
    #endif
    

}

EXP_FUNC int xavaOutputApply(XAVA *hand) {
    // TODO: Fullscreen support
    //if(p->fullF) xdg_toplevel_set_fullscreen(xavaWLSurface, NULL);
    //else        xdg_toplevel_unset_fullscreen(xavaWLSurface);
    
    // process new size
    wl_display_roundtrip(wd.display);

    xavaOutputClear(hand);

    #ifdef EGL
        EGLApply(hand);
    #endif
    #ifdef CAIRO
        __internal_xava_output_cairo_apply(wd.cairo_handle);
    #endif

    return EXIT_SUCCESS;
}

EXP_FUNC XG_EVENT xavaOutputHandleInput(XAVA *hand) {
    UNUSED(hand);
    //XAVA_CONFIG     *p    = &s->conf;

    XG_EVENT event = XAVA_IGNORE;

    while(pendingXAVAEventStack(wd.events)) {
        event = popXAVAEventStack(wd.events);

        switch(event) {
            case XAVA_RESIZE:
                return XAVA_RESIZE;
            case XAVA_QUIT:
                return XAVA_QUIT;
            default:
                break;
        }
    }

    XG_EVENT_STACK *eventStack = 
    #if defined(CAIRO)
        __internal_xava_output_cairo_event(wd.cairo_handle);
    #elif defined(EGL)
        EGLEvent(hand);
    #endif

    while(pendingXAVAEventStack(eventStack)) {
        XG_EVENT event = popXAVAEventStack(eventStack);
        if(event != XAVA_IGNORE)
            return event;
    }

    return event;
}

EXP_FUNC void xavaOutputDraw(XAVA *hand) {
    UNUSED(hand);

    #ifdef EGL
        EGLDraw(hand);
        eglSwapBuffers(wd.ESContext.display, wd.ESContext.surface);
        wl_surface_damage_buffer(wd.surface, 0, 0, INT32_MAX, INT32_MAX);
        wl_surface_commit(wd.surface);
    #endif
    #ifdef CAIRO
        __internal_xava_output_cairo_draw(wd.cairo_handle);
    #endif

    #ifdef SHM
        // signal to wayland about it
        wl_display_roundtrip(wd.display);
    #endif

    wl_display_dispatch_pending(wd.display);
}

EXP_FUNC void xavaOutputLoadConfig(XAVA *hand) {
    XAVA_CONFIG *p = &hand->conf;
    xava_config_source config = hand->default_config.config;

    backgroundLayer = xavaConfigGetBool
        (config, "wayland", "background_layer", 1);
    monitorName = strdup(xavaConfigGetString
        (config, "wayland", "monitor_name", "ignore"));

    #ifdef EGL
        EGLConfigLoad(hand);
    #endif

    #ifdef CAIRO
        wd.cairo_handle = __internal_xava_output_cairo_load_config(hand);
    #endif

    // Vsync is implied, although system timers must be used
    p->vsync = 0;
}

