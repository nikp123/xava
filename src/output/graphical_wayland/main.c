#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <wayland-client-core.h>
#include <wayland-client-protocol.h>
#include <unistd.h>
#include <sys/syscall.h>

#include "../graphical.h"
#include "../../shared.h"

#include "main.h"
#include "render.h"
#include "wl_output.h"
#include "registry.h"
#include "zwlr.h"
#include "xdg.h"

/* Globals */
struct waydata wd;

static _Bool backgroundLayer;
       char* monitorName;

uint32_t fgcol,bgcol;

static void wl_surface_frame_done(void *data, struct wl_callback *cb,
		uint32_t time) {
	struct waydata *wd = data;

	wl_callback_destroy(cb);

	update_frame(wd);

	// request update
	cb = wl_surface_frame(wd->surface);

	wl_callback_add_listener(cb, &wl_surface_frame_listener, wd);

	// signal to wayland about it
	wl_surface_commit(wd->surface);
}
const struct wl_callback_listener wl_surface_frame_listener = {
	.done = wl_surface_frame_done,
};

EXP_FUNC void xavaOutputCleanup(void *v) {
	closeSHM(&wd);

	if(backgroundLayer) {
		zwlr_cleanup(&wd);
	} else {
		xdg_cleanup();
	}
	wl_output_cleanup(&wd);
	wl_surface_destroy(wd.surface);
	wl_compositor_destroy(wd.compositor);
	wl_registry_destroy(xavaWLRegistry);
	wl_display_disconnect(wd.display);
	free(monitorName);
}

EXP_FUNC int xavaInitOutput(struct XAVA_HANDLE *hand) {
	wd.hand   = hand;
	wd.events = newXAVAEventStack();

	wd.display = wl_display_connect(NULL);
	if(wd.display == NULL) {
		fprintf(stderr, "Failed to connect to Wayland server\n");
		return EXIT_FAILURE;
	}

	// Before the registry shananigans, outputs must be initialized
	wl_output_init(&wd);

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
	if(xavaWLRLayerShell == NULL || xavaXDGOutputManager == NULL) {
		fprintf(stderr, "Your compositor doesn't support any of the following:\n"
				"zwlr_layer_shell_v1a and/or zwlr_output_manager_v1\n"
				"This will DISABLE the ability to use the background layer for\n"
				"safety reasons!\n");
		backgroundLayer = 0;
	}

	// needed to be done twice for xdg_output to do it's frickin' job
	wl_display_roundtrip(wd.display);

	wd.surface = wl_compositor_create_surface(wd.compositor);

	// The option carries the same functionality here to Wayland as well
	if(backgroundLayer) {
		zwlr_init(&wd);
	} else {
		xdg_init(&wd);
	}

	//wl_surface_set_buffer_scale(xavaWLSurface, 3);

	// process all of this, FINALLY
	wl_surface_commit(wd.surface);

	wd.shmfd = syscall(SYS_memfd_create, "buffer", 0);

	wd.maxSize = 0;
	wd.fbUnsafe = false;

	reallocSHM(&wd);

	struct wl_callback *cb = wl_surface_frame(wd.surface);
	wl_callback_add_listener(cb, &wl_surface_frame_listener, &wd);

	return EXIT_SUCCESS;
}

EXP_FUNC void xavaOutputClear(struct XAVA_HANDLE *hand) {
	struct config_params *p = &hand->conf;

	if(wd.fbUnsafe) return; 

	wd.fbUnsafe = true;
	for(register int i=0; i<p->w*p->h; i++)
		wd.fb[i] = bgcol;
	wd.fbUnsafe = false;
}

EXP_FUNC int xavaOutputApply(struct XAVA_HANDLE *hand) {
	struct config_params *p = &hand->conf;

	// TODO: Fullscreen support
	//if(p->fullF) xdg_toplevel_set_fullscreen(xavaWLSurface, NULL);
	//else        xdg_toplevel_unset_fullscreen(xavaWLSurface);

	// process new size
	wl_display_roundtrip(wd.display);

	// handle colors
	fgcol = wayland_color_blend(p->col, p->foreground_opacity*255);
	bgcol = wayland_color_blend(p->bgcol, p->background_opacity*255);

	xavaOutputClear(hand);

	return EXIT_SUCCESS;
}

EXP_FUNC XG_EVENT xavaOutputHandleInput(struct XAVA_HANDLE *hand) {
	//struct config_params     *p    = &s->conf;

	XG_EVENT event = XAVA_IGNORE;

	while(pendingXAVAEventStack(wd.events)) {
		event = popXAVAEventStack(wd.events);

		switch(event) {
			case XAVA_RESIZE:
				return XAVA_RESIZE;
			case XAVA_QUIT:
				return XAVA_QUIT;
			default:
				break;
		}
	}

	return event;
}

// super optimized, because cpus are shit at graphics
EXP_FUNC void xavaOutputDraw(struct XAVA_HANDLE *hand) {
	struct config_params     *p    = &hand->conf;

	if(wd.fbUnsafe) return;

	wd.fbUnsafe = true;
	for(int i = 0; i < hand->bars; i++) {
		// get the properly aligned starting pointer
		register uint32_t *fbDataPtr = &wd.fb[hand->rest+i*(p->bs+p->bw)];
		register int bw = p->bw;

		// beginning and end of bars, depends on the order
		register int a = p->h - hand->f[i];
		register int b = p->h - hand->fl[i];
		register uint32_t brush = fgcol;
		if(hand->f[i] < hand->fl[i]) {
			brush = bgcol;
			a^=b; b^=a; a^=b;
		}

		// Update damage only where necessary
		wl_surface_damage_buffer(wd.surface, hand->rest+i*(p->bs+p->bw),
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
	wd.fbUnsafe = false;

	wl_display_roundtrip(wd.display);
}

EXP_FUNC void xavaOutputHandleConfiguration(struct XAVA_HANDLE *hand, void *data) {
	struct config_params *p = &hand->conf;

	dictionary *ini = (dictionary*)data;

	backgroundLayer = iniparser_getboolean
		(ini, "wayland:background_layer", 1);
	monitorName = strdup(iniparser_getstring
		(ini, "wayland:monitor_name", "ignore"));

	// Vsync is implied, although system timers must be used
	p->vsync = 0;
}

