#ifndef __WAYLAND_XDG_SURFACE_H
#define __WAYLAND_XDG_SURFACE_H

#include "gen/xdg-shell-client-protocol.h"

#include "main.h"

extern const struct xdg_surface_listener xdg_surface_listener;
extern const struct xdg_wm_base_listener xdg_wm_base_listener;
extern struct xdg_wm_base *xavaXDGWMBase;

extern void xdg_init(struct surfaceData *s);
extern void xdg_cleanup();

#endif
