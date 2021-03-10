#ifndef __WAYLAND_ZWLR_LAYER_SHELL_H
#define __WAYLAND_ZWLR_LAYER_SHELL_H

#include "gen/wlr-layer-shell-unstable-v1-client-protocol.h"

#include "main.h"

extern const struct zwlr_layer_surface_v1_listener layer_surface_listener;
extern struct zwlr_layer_shell_v1 *xavaWLRLayerShell;

void zwlr_init(struct waydata *s);
void zwlr_cleanup();

#endif
