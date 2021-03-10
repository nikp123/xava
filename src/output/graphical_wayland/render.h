#ifndef __WAYLAND_RENDER_H
#define __WAYLAND_RENDER_H

#include <wayland-client.h>
#include <stdint.h>

#include "../../shared.h"

#include "main.h"

extern _Bool xavaWLCurrentlyDrawing;
extern int xavaWLSHMFD;

uint32_t extern wayland_color_blend(uint32_t color, uint16_t alpha);
void update_frame(struct waydata *s);

#endif
