#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <wayland-client-core.h>
#include <wayland-client-protocol.h>
#include <unistd.h>
#include <sys/syscall.h>

#include "../graphical.h"
#include "../../shared.h"

#include "main.h"
#include "wl_output.h"
#include "registry.h"
#include "zwlr.h"
#include "xdg.h"
#include "egl.h"

/* Globals */
struct waydata wd;

static _Bool backgroundLayer;
       char* monitorName;

uint32_t fgcol,bgcol;

static void wl_surface_frame_done(void *data, struct wl_callback *cb,
        uint32_t time) {
    struct waydata *wd = data;

    wl_callback_destroy(cb);

    // request update
    cb = wl_surface_frame(wd->surface);

    wl_callback_add_listener(cb, &wl_surface_frame_listener, wd);

    // signal to wayland about it
    wl_surface_commit(wd->surface);
}
const struct wl_callback_listener wl_surface_frame_listener = {
    .done = wl_surface_frame_done,
};

EXP_FUNC void xavaOutputCleanup(void *v) {
    waylandEGLDestroy(&wd);

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

EXP_FUNC int xavaInitOutput(struct XAVA_HANDLE *hand) {
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
        xavaWarn("Your compositor doesn't support any of the following:\n"
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

    waylandEGLInit(&wd);
    return EXIT_SUCCESS;
}

EXP_FUNC void xavaOutputClear(struct XAVA_HANDLE *hand) {
    // noop
}

EXP_FUNC int xavaOutputApply(struct XAVA_HANDLE *hand) {
    // TODO: Fullscreen support
    //if(p->fullF) xdg_toplevel_set_fullscreen(xavaWLSurface, NULL);
    //else        xdg_toplevel_unset_fullscreen(xavaWLSurface);

    // process new size
    wl_display_roundtrip(wd.display);

    xavaOutputClear(hand);

    waylandEGLApply(hand);

    return EXIT_SUCCESS;
}

EXP_FUNC XG_EVENT xavaOutputHandleInput(struct XAVA_HANDLE *hand) {
    //struct config_params     *p    = &s->conf;

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

    if(waylandEGLEvent(&wd) == XAVA_RELOAD) {
        return XAVA_RELOAD;
    }

    return event;
}

// super optimized, because cpus are shit at graphics
EXP_FUNC void xavaOutputDraw(struct XAVA_HANDLE *hand) {
    waylandEGLDraw(hand);

    wl_display_dispatch_pending(wd.display);
}

EXP_FUNC void xavaOutputLoadConfig(struct XAVA_HANDLE *hand) {
    struct config_params *p = &hand->conf;
    XAVACONFIG config = hand->default_config.config;

    backgroundLayer = xavaConfigGetBool
        (config, "wayland", "background_layer", 1);
    monitorName = strdup(xavaConfigGetString
        (config, "wayland", "monitor_name", "ignore"));

    waylandEGLConfigLoad(hand);

    // Vsync is implied, although system timers must be used
    p->vsync = 0;
}

