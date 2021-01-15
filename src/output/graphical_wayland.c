#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <wayland-client-protocol.h>
#include <unistd.h>
#include <sys/syscall.h>
#include <sys/mman.h>

#include "xdg-shell-client-protocol.h"

#include "graphical.h"
#include "../config.h"
#include "graphical_wayland.h"

/* Globals */
struct wl_display *xavaWLDisplay;
struct wl_registry *xavaWLRegistry;
struct wl_shm *xavaWLSHM;
struct wl_compositor *xavaWLCompositor;
struct xdg_wm_base *xavaXDGWMBase;
/* Objects */
struct wl_surface *xavaWLSurface;
struct xdg_surface *xavaXDGSurface;
struct xdg_toplevel *xavaXDGToplevel;
struct wl_shm_pool *xavaWLSHMPool;
int shmFileIndentifier;

_Bool xavaWLCurrentlyDrawing = 0;
uint32_t *xavaWLFrameBuffer;
int xavaWLSHMFD;

static void wl_buffer_release(void *data, struct wl_buffer *wl_buffer) {
	/* Sent by the compositor when it's no longer using this buffer */
	wl_buffer_destroy(wl_buffer);
}
static const struct wl_buffer_listener wl_buffer_listener = {
	.release = wl_buffer_release,
};

struct wl_buffer *wl_create_framebuffer(void) {
	int width = p.w, height = p.h;
	int stride = width*4;
	int size = stride * height;

	struct wl_shm_pool *pool = wl_shm_create_pool(xavaWLSHM, xavaWLSHMFD, size);
	struct wl_buffer *buffer = wl_shm_pool_create_buffer(pool,
			0, width, height, stride, WL_SHM_FORMAT_ARGB8888);
	wl_shm_pool_destroy(pool);

	wl_buffer_add_listener(buffer, &wl_buffer_listener, NULL);
	return buffer;
}

static const struct wl_callback_listener wl_surface_frame_listener;
static void wl_surface_frame_done(void *data, struct wl_callback *cb, uint32_t time) {
	/* Destroy this callback */
	wl_callback_destroy(cb);

	/* Request another frame */
	cb = wl_surface_frame(xavaWLSurface);
	wl_callback_add_listener(cb, &wl_surface_frame_listener, NULL);

	/* Submit a frame for this event */
	struct wl_buffer *buffer = wl_create_framebuffer();
	wl_surface_attach(xavaWLSurface, buffer, 0, 0);
	wl_surface_damage_buffer(xavaWLSurface, 0, 0, INT32_MAX, INT32_MAX);
	wl_surface_commit(xavaWLSurface);
}
static const struct wl_callback_listener wl_surface_frame_listener = {
	.done = wl_surface_frame_done,
};

static void xdg_surface_configure(void *data, struct xdg_surface *xdg_surface,
		uint32_t serial) {
	xdg_surface_ack_configure(xdg_surface, serial);
	wl_create_framebuffer();

	struct wl_buffer *buffer = wl_create_framebuffer();
	while(xavaWLCurrentlyDrawing) { usleep(1000); }; // wait for Vsync kind of deal

	wl_surface_attach(xavaWLSurface, buffer, 0, 0);
	wl_surface_commit(xavaWLSurface);
}
static const struct xdg_surface_listener xdg_surface_listener = {
	.configure = xdg_surface_configure,
};

static void xdg_wm_base_ping(void *data, struct xdg_wm_base *xdg_wm_base,
		uint32_t serial) {
	xdg_wm_base_pong(xdg_wm_base, serial);
}
static const struct xdg_wm_base_listener xdg_wm_base_listener = {
	.ping = xdg_wm_base_ping,
};

static void xava_wl_registry_global_listener(void *data, struct wl_registry *wl_registry,
		uint32_t name, const char *interface, uint32_t version) {
	if (strcmp(interface, wl_shm_interface.name) == 0) {
		xavaWLSHM = wl_registry_bind(
			wl_registry, name, &wl_shm_interface, 1);
	} else if (strcmp(interface, wl_compositor_interface.name) == 0) {
		xavaWLCompositor = wl_registry_bind(
			wl_registry, name, &wl_compositor_interface, 4);
	} else if (strcmp(interface, xdg_wm_base_interface.name) == 0) {
		xavaXDGWMBase = wl_registry_bind(
			wl_registry, name, &xdg_wm_base_interface, 1);
		xdg_wm_base_add_listener(xavaXDGWMBase,
			&xdg_wm_base_listener, NULL);
	}
}

static void xava_wl_registry_global_remove(void *data, struct wl_registry *wl_registry,
		uint32_t name) {
	// noop
}

static const struct wl_registry_listener xava_wl_registry_listener = {
	.global = xava_wl_registry_global_listener,
	.global_remove = xava_wl_registry_global_remove,
};

void cleanup_graphical_wayland(void) {

}


void handle_wayland_platform_quirks(void) {
	// Vsync is implied in Wayland
	// it is disabled because it messes with the timing code
	// in the backend
	if(p.vsync) p.vsync = 0;
}

int init_window_wayland(void) {
	handle_wayland_platform_quirks();

	xavaWLDisplay = wl_display_connect(NULL);
	if(xavaWLDisplay == NULL) {
		fprintf(stderr, "Failed to connect to Wayland server\n");
		return EXIT_FAILURE;
	}

	xavaWLRegistry = wl_display_get_registry(xavaWLDisplay);
	// TODO: Check failure states
	wl_registry_add_listener(xavaWLRegistry, &xava_wl_registry_listener, NULL);
	wl_display_roundtrip(xavaWLDisplay);
	if(xavaWLSHM == NULL) {
		fprintf(stderr, "Your compositor doesn't support wl_shm, failing....\n");
		return EXIT_FAILURE;
	}
	if(xavaWLCompositor == NULL) {
		fprintf(stderr, "Your compositor doesn't support wl_compositor, failing....\n");
		return EXIT_FAILURE;
	}
	if(xavaXDGWMBase == NULL) {
		fprintf(stderr, "Your compositor doesn't support xdg_wm_base, failing....\n");
		return EXIT_FAILURE;
	}

	xavaWLSurface = wl_compositor_create_surface(xavaWLCompositor);
	// TODO: Check failure states
	xavaXDGSurface = xdg_wm_base_get_xdg_surface(xavaXDGWMBase, xavaWLSurface);
	// TODO: Check failure states
	xdg_surface_add_listener(xavaXDGSurface, &xdg_surface_listener, NULL);
	xavaXDGToplevel = xdg_surface_get_toplevel(xavaXDGSurface);
	// TODO: Check failure states
	xdg_toplevel_set_title(xavaXDGToplevel, "XAVA");

	struct wl_callback *cb = wl_surface_frame(xavaWLSurface);
	wl_callback_add_listener(cb, &wl_surface_frame_listener, NULL);

	wl_surface_commit(xavaWLSurface);
	return EXIT_SUCCESS;
}

void clear_screen_wayland(void) {
	close(xavaWLSHMFD);
	munmap(xavaWLFrameBuffer, p.w*p.h*sizeof(uint32_t));
}

int apply_window_settings_wayland(void) {
	int size = p.w*p.h*sizeof(uint32_t);
	xavaWLSHMFD = syscall(SYS_memfd_create, "buffer", 0);
	ftruncate(xavaWLSHMFD, size);

	xavaWLFrameBuffer = mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_SHARED, xavaWLSHMFD, 0);
	if(xavaWLFrameBuffer == MAP_FAILED) {
		close(xavaWLSHMFD);
		fprintf(stderr, "Failed to create a shared memory buffer\n");
		return EXIT_FAILURE;
	}

	return EXIT_SUCCESS;
}

int get_window_input_wayland(void) {
	return 0;
}

void draw_graphical_wayland(int bars, int rest, int *f, int *flastd) {
	xavaWLCurrentlyDrawing = 1;
	for(int i = 0; i < bars; i++) {
		// get the properly aligned starting pointer
		register uint32_t *fbDataPtr = &xavaWLFrameBuffer[rest+i*(p.bs+p.bw)];
		register int bw = p.bw;

		// beginning and end of bars, depends on the order
		register int a = p.h - f[i];
		register int b = p.h - flastd[i];
		register uint32_t brush = p.col;
		if(f[i] < flastd[i]) {
			brush = p.bgcol;
			a^=b; b^=a; a^=b;
		}

		// advance the pointer by undrawn pixels amount
		fbDataPtr+=a*p.w;

		// loop through the rows of pixels
		for(register int j = a; j < b; j++) {
			for(register int k = 0; k < bw; k++) {
				*fbDataPtr++ = brush;
			}
			fbDataPtr += (p.w-p.bw);
		}
	}
	xavaWLCurrentlyDrawing = 0;
	wl_display_roundtrip(xavaWLDisplay);
}

