#include <EGL/eglplatform.h>
#include <stdint.h>
#include <EGL/egl.h>
#include <wayland-egl.h>
#include <wayland-egl-core.h>

#include "output/shared/graphical.h"
#include "output/shared/gl/egl.h"

#include "egl.h"
#include "main.h"

void waylandEGLCreateWindow(struct waydata *wd) {
    //region = wl_compositor_create_region(wd->compositor);
    //wl_region_add(region, 0, 0, width, height);
    //wl_surface_set_opaque_region(surface, region);

    struct wl_egl_window *egl_window = wl_egl_window_create(wd->surface,
            wd->hand->conf.w, wd->hand->conf.h);

    xavaBailCondition(egl_window == EGL_NO_SURFACE,
            "Failed to create EGL window!\n");

    xavaSpam("Created EGL window!");

    wd->ESContext.native_window = (EGLNativeWindowType)egl_window;
    wd->ESContext.native_display = wd->display;
}

void waylandEGLWindowResize(struct waydata *wd, int w, int h) {
    wl_egl_window_resize((struct wl_egl_window*)wd->ESContext.native_window,
                    w, h, 0, 0);
    wl_surface_commit(wd->surface);
}

