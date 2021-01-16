#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <wayland-client-protocol.h>
#include <unistd.h>
#include <sys/syscall.h>
#include <sys/mman.h>

#include "xdg-shell-client-protocol.h"
#include "wlr-layer-shell-unstable-v1-client-protocol.h"
#include "xdg-output-unstable-v1-client-protocol.h"
#include "wlr-output-managment-unstable-v1.h"

#include "graphical.h"
#include "../config.h"
#include "graphical_wayland.h"

/* Globals */
struct wl_display *xavaWLDisplay;
struct wl_registry *xavaWLRegistry;
struct wl_shm *xavaWLSHM;
struct wl_compositor *xavaWLCompositor;
struct xdg_wm_base *xavaXDGWMBase;
// monitor/display managment crap
struct zwlr_layer_shell_v1 *xavaWLRLayerShell;
struct xdg_output_manager *xavaXDGOutputManager;
/* Objects */
struct wl_surface *xavaWLSurface;
struct xdg_surface *xavaXDGSurface;
struct xdg_toplevel *xavaXDGToplevel;
struct wl_shm_pool *xavaWLSHMPool;
// monitor/display managment crap
struct zwlr_layer_surface_v1 *xavaWLRLayerSurface;
struct wl_output *xavaWLOutput;

int shmFileIndentifier;
_Bool xavaWLCurrentlyDrawing = 0;
uint32_t *xavaWLFrameBuffer;
int xavaWLSHMFD;

int width_margin, height_margin;

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

static void update_frame() {
	// Vsync kind of thing here
	while(xavaWLCurrentlyDrawing) usleep(1000);

	// Update frame and inform wayland 
	struct wl_buffer *buffer = wl_create_framebuffer();
	wl_surface_attach(xavaWLSurface, buffer, 0, 0);
	//wl_surface_damage_buffer(xavaWLSurface, 0, 0, INT32_MAX, INT32_MAX);
	wl_surface_commit(xavaWLSurface);
}

static void layer_surface_configure(void *data,
		struct zwlr_layer_surface_v1 *surface,
		uint32_t serial, uint32_t width, uint32_t height) {
	//screenWidth = width;
	//screenHeight = height;

	// Respond to compositor
	zwlr_layer_surface_v1_ack_configure(surface, serial);

	update_frame();
}
static void layer_surface_closed(void *data,
		struct zwlr_layer_surface_v1 *surface) {
	//destroy_swaybg_output(output);
}
static const struct zwlr_layer_surface_v1_listener layer_surface_listener = {
	.configure = layer_surface_configure,
	.closed = layer_surface_closed,
};

static const struct wl_callback_listener wl_surface_frame_listener;
static void wl_surface_frame_done(void *data, struct wl_callback *cb, uint32_t time) {
	/* Destroy this callback */
	wl_callback_destroy(cb);

	/* Request another frame */
	cb = wl_surface_frame(xavaWLSurface);
	wl_callback_add_listener(cb, &wl_surface_frame_listener, NULL);

	update_frame();
}
static const struct wl_callback_listener wl_surface_frame_listener = {
	.done = wl_surface_frame_done,
};

static void xdg_surface_configure(void *data, struct xdg_surface *xdg_surface,
		uint32_t serial) {
	// confirm that you exist to the compositor
	xdg_surface_ack_configure(xdg_surface, serial);

	update_frame();
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
	} else if (strcmp(interface, zwlr_layer_shell_v1_interface.name) == 0) {
		xavaWLRLayerShell = wl_registry_bind(xavaWLRegistry, name,
				&zwlr_layer_shell_v1_interface, 1);
	} else if (strcmp(interface, zwlr_output_manager_v1_interface.name) == 0) {
		xavaXDGOutputManager = wl_registry_bind(xavaWLRegistry, name,
			&zwlr_output_manager_v1_interface, 2);
	} else if (strcmp(interface, wl_output_interface.name) == 0) {
		// TODO: Multi-monitor support
		//output->state = state;
		//output->wl_name = name;
		xavaWLOutput = wl_registry_bind(xavaWLRegistry, name,
				&wl_output_interface, 3);
		//wl_output_add_listener(output->wl_output, &output_listener, output);
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
	close(xavaWLSHMFD);
	munmap(xavaWLFrameBuffer, p.w*p.h*sizeof(uint32_t));

	if(p.overrideRedirect) {
		wl_output_destroy(xavaWLOutput);
		zwlr_layer_surface_v1_destroy(xavaWLRLayerSurface);
		zwlr_layer_shell_v1_destroy(xavaWLRLayerShell);
	} else {
		xdg_toplevel_destroy(xavaXDGToplevel);
		xdg_surface_destroy(xavaXDGSurface);
	}
	wl_surface_destroy(xavaWLSurface);
	wl_compositor_destroy(xavaWLCompositor);
	wl_registry_destroy(xavaWLRegistry);
	wl_display_disconnect(xavaWLDisplay);
}

void handle_wayland_platform_quirks(void) {
	// Vsync is implied in Wayland
	// it is disabled because it messes with the timing code
	// in the backend
	if(p.vsync) p.vsync = 0;
}

/**
 * calculate and correct window positions
 * NOTE: The global calculate_win_pos() doesn't provide
 * enough functionality for this to work
 * It assumes clients are able to pick their own positions
**/
uint32_t handle_window_alignment(void) {
	const uint32_t top = ZWLR_LAYER_SURFACE_V1_ANCHOR_TOP;
	const uint32_t bottom = ZWLR_LAYER_SURFACE_V1_ANCHOR_BOTTOM;
	const uint32_t left = ZWLR_LAYER_SURFACE_V1_ANCHOR_LEFT;
	const uint32_t right = ZWLR_LAYER_SURFACE_V1_ANCHOR_RIGHT;

	uint32_t anchor = 0;
	width_margin = p.wx;
	height_margin = p.wy;

	// margins are reversed on opposite edges for user convenience

	// Top alingments
	if(!strcmp("top", p.winA)) anchor = top; 
	else if(!strcmp("top_left", p.winA)) anchor = top|left; 
	else if(!strcmp("top_right", p.winA)) {
		width_margin *= -1; anchor = top|right;
	}

	// Middle alignments
	else if(!strcmp("left", p.winA)) anchor = left;
	else if(!strcmp("center", p.winA)) { /** nop **/ }
	else if(!strcmp("right", p.winA)) { 
		width_margin *= -1; anchor = right;
	}
	// Bottom alignments
	else if(!strcmp("bottom_left", p.winA)) anchor = left|bottom;
	else if(!strcmp("bottom", p.winA)) anchor = bottom;
	else if(!strcmp("bottom_right", p.winA)) { 
		width_margin *= -1; anchor = right|bottom;
	}

	if(!strncmp("bottom", p.winA, 6))
		height_margin *= -1;

	return anchor;
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
	if(xavaWLRLayerShell == NULL || xavaXDGOutputManager == NULL || xavaWLOutput == NULL) {
		fprintf(stderr, "Your compositor doesn't support any of the following:\n"
				"zwlr_layer_shell_v1, zwlr_output_manager_v1 and/or wl_output\n"
				"This will DISABLE the ability to use override_redirect for safety"
				" reasons!\n");
		p.overrideRedirect = 0;
	}

	xavaWLSurface = wl_compositor_create_surface(xavaWLCompositor);

	// p.overrideRedirect is a carry-over from X11
	// It is basically a feature that allows windows to be
	// rendered in the background in most supported window managers
	//
	// The option carries the same functionality here to Wayland as well
	if(p.overrideRedirect) {
		// Create a "wallpaper" surface
		xavaWLRLayerSurface = zwlr_layer_shell_v1_get_layer_surface(
			xavaWLRLayerShell, xavaWLSurface, xavaWLOutput,
			ZWLR_LAYER_SHELL_V1_LAYER_BOTTOM, "bottom");

		// adjust position and properties accordingly
		zwlr_layer_surface_v1_set_size(xavaWLRLayerSurface, p.w, p.h);
		zwlr_layer_surface_v1_set_anchor(xavaWLRLayerSurface, handle_window_alignment());
		zwlr_layer_surface_v1_set_margin(xavaWLRLayerSurface, 
				height_margin, -width_margin, -height_margin, width_margin);
		zwlr_layer_surface_v1_set_exclusive_zone(xavaWLRLayerSurface, -1);

		// same stuff as xdg_surface_add_listener, but for zwlr_layer_surface
		zwlr_layer_surface_v1_add_listener(xavaWLRLayerSurface,
			&layer_surface_listener, NULL);
	} else {
		// create window, or "surface" in waland terms
		xavaXDGSurface = xdg_wm_base_get_xdg_surface(xavaXDGWMBase, xavaWLSurface);

		// for those unaware, the compositor baby sits everything that you
		// make, thus it needs a function through which the compositor
		// will manage your application
		xdg_surface_add_listener(xavaXDGSurface, &xdg_surface_listener, NULL);

		xavaXDGToplevel = xdg_surface_get_toplevel(xavaXDGSurface);
		xdg_toplevel_set_title(xavaXDGToplevel, "XAVA");
	}

	// This callback basically just updates the frames
	struct wl_callback *cb = wl_surface_frame(xavaWLSurface);
	wl_callback_add_listener(cb, &wl_surface_frame_listener, NULL);

	// process all of this, FINALLY
	wl_surface_commit(xavaWLSurface);
	return EXIT_SUCCESS;
}

void clear_screen_wayland(void) {
	for(register int i=0; i<p.w*p.h; i++)
		xavaWLFrameBuffer[i] = p.bgcol;
}

int apply_window_settings_wayland(void) {
	// TODO: Fullscreen support
	//if(p.fullF) xdg_toplevel_set_fullscreen(xavaWLSurface, NULL);
	//else        xdg_toplevel_unset_fullscreen(xavaWLSurface);

	int size = p.w*p.h*sizeof(uint32_t);
	xavaWLSHMFD = syscall(SYS_memfd_create, "buffer", 0);
	ftruncate(xavaWLSHMFD, size);

	xavaWLFrameBuffer = mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_SHARED, xavaWLSHMFD, 0);
	if(xavaWLFrameBuffer == MAP_FAILED) {
		close(xavaWLSHMFD);
		fprintf(stderr, "Failed to create a shared memory buffer\n");
		return EXIT_FAILURE;
	}

	// handle colors
	p.col = (p.col&0x00ffffff) | ((uint32_t)
		(p.foreground_opacity*0xff)<<24);
	p.bgcol = (p.bgcol&0x00ffffff) | ((uint32_t)
		(p.background_opacity*0xff)<<24);

	// clean screen because the colors changed
	clear_screen_wayland();

	return EXIT_SUCCESS;
}

int get_window_input_wayland(void) {
	// i am too lazy to do this part
	// especially with how tedious Wayland is for client-side
	// development

	// obligatory highlighted TODO goes here
	return 0;
}

// super optimized, because cpus are shit at graphics
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

		// Update damage only where necessary
		wl_surface_damage_buffer(xavaWLSurface, rest+i*(p.bs+p.bw),
				a, p.bw, b-a);

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

