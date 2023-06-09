#include <cairo.h>

#ifndef CAIRO
    #define CAIRO
#endif

#ifndef SHM
    #define SHM
#endif

#include "main.h"
#include "output/shared/cairo/main.h"

void xava_output_wayland_cairo_init(struct waydata *wd) {
    wd->cairo_surface = cairo_image_surface_create_for_data(wd->shm.buffer,
        CAIRO_FORMAT_ARGB32, wd->shm.dim.w, wd->shm.dim.h, wd->shm.dim.stride);
    __internal_xava_output_cairo_init(wd->cairo_handle,
            cairo_create(wd->cairo_surface));
}

void xava_output_wayland_cairo_resize(struct waydata *wd) {
    cairo_surface_destroy(wd->cairo_surface);
    wd->cairo_surface = cairo_image_surface_create_for_data(wd->shm.buffer,
        CAIRO_FORMAT_ARGB32, wd->shm.dim.w, wd->shm.dim.h, wd->shm.dim.stride);
    wd->cairo_handle->cr = cairo_create(wd->cairo_surface);
}
