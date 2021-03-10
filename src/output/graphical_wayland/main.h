#ifndef __WAYLAND_MAIN_H
#define __WAYLAND_MAIN_H

// exported function, a macro used to determine which functions
// are exposed as symbols within the final library/obj files
#define EXP_FUNC __attribute__ ((visibility ("default")))

#include <wayland-client.h>

#include "../../shared.h" 

struct waydata {
	struct wl_surface    *surface;
	struct wl_shm        *shm;
	struct wl_display    *display;
	struct wl_compositor *compositor;
	struct state_params  *s;
	XG_EVENT event;
};


extern int monitorNumber;

#endif
