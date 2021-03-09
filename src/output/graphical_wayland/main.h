#ifndef __WAYLAND_MAIN_H
#define __WAYLAND_MAIN_H

// exported function, a macro used to determine which functions
// are exposed as symbols within the final library/obj files
#define EXP_FUNC __attribute__ ((visibility ("default")))

#include <wayland-client.h>

#include "../../shared.h" 

struct surfaceData {
	struct wl_surface *surface;
	struct state_params *s;
};

extern struct wl_shm *xavaWLSHM;
extern struct wl_display *xavaWLDisplay;
extern struct wl_compositor *xavaWLCompositor;

extern XG_EVENT storedEvent;
extern int monitorNumber;

#endif
