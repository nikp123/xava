#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <wayland-client-protocol.h>
#include <unistd.h>
#include <sys/syscall.h>
#include <sys/mman.h>

#include "../graphical.h"
#include "../../shared.h"

#include "main.h"
#include "render.h"
#include "wl_output.h"
#include "registry.h"
#include "zwlr.h"
#include "xdg.h"

/* Globals */
//static struct wl_shm_pool *xavaWLSHMPool;
 
static uint32_t *xavaWLFrameBuffer;
 
struct waydata wd;

static _Bool backgroundLayer;
int monitorNumber;

uint32_t fgcol,bgcol;

static const struct wl_callback_listener wl_surface_frame_listener;
static void wl_surface_frame_done(void *data, struct wl_callback *cb,
		uint32_t time) {
	struct waydata *wd = data;

	wl_callback_destroy(cb);

	// stop updating frames while XAVA's having a nice sleep
	while(wd->s->pauseRendering) 
		usleep(10000);

	update_frame(wd);

	// request update
	cb = wl_surface_frame(wd->surface);
	wl_callback_add_listener(cb, &wl_surface_frame_listener, wd);

	// signal to wayland about it
	wl_surface_commit(wd->surface);
}
static const struct wl_callback_listener wl_surface_frame_listener = {
	.done = wl_surface_frame_done,
};

void closeSHM(void *v) {
	struct state_params *s = v;
	struct config_params *p = &s->conf;

	close(xavaWLSHMFD);
	munmap(xavaWLFrameBuffer, p->w*p->h*sizeof(uint32_t));
}

EXP_FUNC void xavaOutputCleanup(void *v) {
	struct state_params *s = v;
	closeSHM(s);

	if(backgroundLayer) {
		zwlr_cleanup();
		wl_output_cleanup();
	} else {
		xdg_cleanup();
	}
	wl_surface_destroy(wd.surface);
	wl_compositor_destroy(wd.compositor);
	wl_registry_destroy(xavaWLRegistry);
	wl_display_disconnect(wd.display);
}

EXP_FUNC int xavaInitOutput(void *v) {
	wd.s       = v;

	wd.display = wl_display_connect(NULL);
	if(wd.display == NULL) {
		fprintf(stderr, "Failed to connect to Wayland server\n");
		return EXIT_FAILURE;
	}

	xavaWLRegistry = wl_display_get_registry(wd.display);
	// TODO: Check failure states
	wl_registry_add_listener(xavaWLRegistry, &xava_wl_registry_listener, &wd);
	wl_display_roundtrip(wd.display);
	if(wd.shm == NULL) {
		fprintf(stderr, "Your compositor doesn't support wl_shm, failing....\n");
		return EXIT_FAILURE;
	}
	if(wd.compositor == NULL) {
		fprintf(stderr, "Your compositor doesn't support wl_compositor, failing....\n");
		return EXIT_FAILURE;
	}
	if(xavaXDGWMBase == NULL) {
		fprintf(stderr, "Your compositor doesn't support xdg_wm_base, failing....\n");
		return EXIT_FAILURE;
	}
	if(xavaWLRLayerShell == NULL || xavaXDGOutputManager == NULL || xavaWLOutputs == NULL) {
		fprintf(stderr, "Your compositor doesn't support any of the following:\n"
				"zwlr_layer_shell_v1, zwlr_output_manager_v1 and/or wl_output\n"
				"This will DISABLE the ability to use the background layer for\n"
				"safety reasons!\n");
		backgroundLayer = 0;
	}

	wd.surface = wl_compositor_create_surface(wd.compositor);

	// The option carries the same functionality here to Wayland as well
	if(backgroundLayer) {
		zwlr_init(&wd);
	} else {
		xdg_init(&wd);
	}

	//wl_surface_set_buffer_scale(xavaWLSurface, 3);

	struct wl_callback *cb = wl_surface_frame(wd.surface);
	wl_callback_add_listener(cb, &wl_surface_frame_listener, &wd);

	// process all of this, FINALLY
	wl_surface_commit(wd.surface);

	xavaWLSHMFD = syscall(SYS_memfd_create, "buffer", 0);
	return EXIT_SUCCESS;
}

EXP_FUNC void xavaOutputClear(void *v) {
	struct state_params *s = v;
	struct config_params *p = &s->conf;

	for(register int i=0; i<p->w*p->h; i++)
		xavaWLFrameBuffer[i] = bgcol;
}

EXP_FUNC int xavaOutputApply(void *v) {
	struct state_params *s = v;
	struct config_params *p = &s->conf;

	// TODO: Fullscreen support
	//if(p->fullF) xdg_toplevel_set_fullscreen(xavaWLSurface, NULL);
	//else        xdg_toplevel_unset_fullscreen(xavaWLSurface);

	int size = p->w*p->h*sizeof(uint32_t);
	//xavaWLSHMFD = syscall(SYS_memfd_create, "buffer", 0);
	ftruncate(xavaWLSHMFD, size);

	xavaWLFrameBuffer = mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_SHARED, xavaWLSHMFD, 0);
	if(xavaWLFrameBuffer == MAP_FAILED) {
		close(xavaWLSHMFD);
		fprintf(stderr, "Failed to create a shared memory buffer\n");
		return EXIT_FAILURE;
	}

	// handle colors
	fgcol = wayland_color_blend(p->col, p->foreground_opacity*255);
	bgcol = wayland_color_blend(p->bgcol, p->background_opacity*255);

	// clean screen because the colors changed
	xavaOutputClear(v);

	return EXIT_SUCCESS;
}

EXP_FUNC XG_EVENT xavaOutputHandleInput(void *v) {
	struct state_params *s = (struct state_params*)v;

	XG_EVENT event = wd.event;
	wd.event = XAVA_IGNORE;

	switch(wd.event) {
		case XAVA_RESIZE:
			closeSHM(s);
			break;
		default:
			break;
	}

	if(event != XAVA_IGNORE)
		return event;

	// obligatory highlighted TODO goes here
	return XAVA_IGNORE;
}

// super optimized, because cpus are shit at graphics
EXP_FUNC void xavaOutputDraw(void *v, int bars, int rest, int *f, int *flastd) {
	struct state_params *s = v;
	struct config_params *p = &s->conf;

	xavaWLCurrentlyDrawing = 1;
	for(int i = 0; i < bars; i++) {
		// get the properly aligned starting pointer
		register uint32_t *fbDataPtr = &xavaWLFrameBuffer[rest+i*(p->bs+p->bw)];
		register int bw = p->bw;

		// beginning and end of bars, depends on the order
		register int a = p->h - f[i];
		register int b = p->h - flastd[i];
		register uint32_t brush = fgcol;
		if(f[i] < flastd[i]) {
			brush = bgcol;
			a^=b; b^=a; a^=b;
		}

		// Update damage only where necessary
		wl_surface_damage_buffer(wd.surface, rest+i*(p->bs+p->bw),
				a, p->bw, b-a);

		// advance the pointer by undrawn pixels amount
		fbDataPtr+=a*p->w;

		// loop through the rows of pixels
		for(register int j = a; j < b; j++) {
			for(register int k = 0; k < bw; k++) {
				*fbDataPtr++ = brush;
			}
			fbDataPtr += (p->w-p->bw);
		}
	}
	xavaWLCurrentlyDrawing = 0;

	wl_display_roundtrip(wd.display);
}

EXP_FUNC void xavaOutputHandleConfiguration(void *v, void *data) {
	struct state_params *s = v;
	struct config_params *p = &s->conf;

	dictionary *ini = (dictionary*)data;

	backgroundLayer = iniparser_getboolean
		(ini, "wayland:background_layer", 0);
	monitorNumber = iniparser_getboolean
		(ini, "wayland:monitor_num", 0);

	// Vsync is implied, although system timers must be used
	p->vsync = 0;
}

