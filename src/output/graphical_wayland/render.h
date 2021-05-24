#ifndef __WAYLAND_RENDER_H
#define __WAYLAND_RENDER_H

#include <wayland-client.h>
#include <stdint.h>

#include "../../shared.h"

#include "main.h"

extern uint32_t wayland_color_blend (uint32_t color, uint8_t alpha);
extern void     update_frame        (struct waydata *s);
extern void     reallocSHM          (struct waydata *wd);
extern void     closeSHM            (struct waydata *wd);

#endif
