#ifndef __WAYLAND_REGISTRY_H
#define __WAYLAND_REGISTRY_H

#include <wayland-client.h>

#include "../../shared.h"

extern struct wl_registry *xavaWLRegistry;
extern const struct wl_registry_listener xava_wl_registry_listener;

#endif
