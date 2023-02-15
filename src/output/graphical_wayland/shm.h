#ifndef __WAYLAND_SHM_H
#define __WAYLAND_SHM_H

#include <wayland-client.h>
#include <stdint.h>

#include "shared.h"

#include "main.h"

uint32_t wayland_color_blend (uint32_t color, uint8_t alpha);
void     update_frame        (struct waydata *wd);
void     reallocSHM          (struct waydata *wd);
void     closeSHM            (struct waydata *wd);
void     wl_surface_frame_done(void *data,
                               struct wl_callback *cb,
                               uint32_t time);

extern const struct wl_callback_listener wl_surface_frame_listener;

#endif

