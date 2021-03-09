#ifndef __WAYLAND_WL_OUTPUT_H
#define __WAYLAND_WL_OUTPUT_H

#include "gen/xdg-output-unstable-v1-client-protocol.h"
#include "gen/wlr-output-managment-unstable-v1.h"

struct wlOutput {
	struct wl_output *output;
	uint32_t scale;
	uint32_t name;
	uint32_t num;
};

extern struct xdg_output_manager *xavaXDGOutputManager;
extern const struct wl_output_listener output_listener;
extern struct wlOutput **xavaWLOutputs;
extern int xavaWLOutputsCount;

struct wlOutput *wl_output_get_desired();
void wl_output_cleanup();

#endif
