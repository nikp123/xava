#ifndef __WAYLAND_MAIN_H
#define __WAYLAND_MAIN_H

// exported function, a macro used to determine which functions
// are exposed as symbols within the final library/obj files
#define EXP_FUNC __attribute__ ((visibility ("default")))

#ifdef EGL
	#include <stdint.h>
	#include <EGL/egl.h>
	#include <GLES2/gl2.h>
#endif

#include <wayland-client.h>

#include "../../shared.h" 

struct waydata {
	struct wl_surface    *surface;
	struct wl_shm        *shm;
	struct wl_display    *display;
	struct wl_compositor *compositor;
	struct XAVA_HANDLE   *hand;
	XG_EVENT_STACK       *events;
	uint32_t             maxSize;
	uint32_t             *fb;
	int                  shmfd;
	_Bool                fbUnsafe;
	struct wl_list       outputs;
};


extern const struct wl_callback_listener wl_surface_frame_listener;

extern char* monitorName;

EXP_FUNC void xavaOutputClear(struct XAVA_HANDLE *hand);

#endif
