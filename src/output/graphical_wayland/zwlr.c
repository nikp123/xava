#include "../../shared.h"

#include "zwlr.h"
#include "render.h"
#include "main.h"
#include "wl_output.h"

static struct zwlr_layer_surface_v1 *xavaWLRLayerSurface;
static int width_margin, height_margin;

struct zwlr_layer_shell_v1 *xavaWLRLayerShell;

static void layer_surface_configure(void *data,
		struct zwlr_layer_surface_v1 *surface,
		uint32_t serial, uint32_t width, uint32_t height) {
	struct surfaceData *s = data;

	//screenWidth = width;
	//screenHeight = height;

	// Respond to compositor
	zwlr_layer_surface_v1_ack_configure(surface, serial);

	update_frame(s);
}

static void layer_surface_closed(void *data,
		struct zwlr_layer_surface_v1 *surface) {
	//destroy_swaybg_output(output);
	#ifdef DEBUG
		fprintf(stderr, "wayland: zwlr_layer_surface lost\n");
	#endif
}

const struct zwlr_layer_surface_v1_listener layer_surface_listener = {
	.configure = layer_surface_configure,
	.closed = layer_surface_closed,
};

/**
 * calculate and correct window positions
 * NOTE: The global calculate_win_pos() doesn't provide
 * enough functionality for this to work
 * It assumes clients are able to pick their own positions
**/
extern uint32_t handle_window_alignment(struct config_params *p) {
	const uint32_t top = ZWLR_LAYER_SURFACE_V1_ANCHOR_TOP;
	const uint32_t bottom = ZWLR_LAYER_SURFACE_V1_ANCHOR_BOTTOM;
	const uint32_t left = ZWLR_LAYER_SURFACE_V1_ANCHOR_LEFT;
	const uint32_t right = ZWLR_LAYER_SURFACE_V1_ANCHOR_RIGHT;

	uint32_t anchor = 0;
	width_margin = p->wx;
	height_margin = p->wy;

	// margins are reversed on opposite edges for user convenience

	// Top alingments
	if(!strcmp("top", p->winA)) anchor = top; 
	else if(!strcmp("top_left", p->winA)) anchor = top|left; 
	else if(!strcmp("top_right", p->winA)) {
		width_margin *= -1; anchor = top|right;
	}

	// Middle alignments
	else if(!strcmp("left", p->winA)) anchor = left;
	else if(!strcmp("center", p->winA)) { /** nop **/ }
	else if(!strcmp("right", p->winA)) { 
		width_margin *= -1; anchor = right;
	}
	// Bottom alignments
	else if(!strcmp("bottom_left", p->winA)) anchor = left|bottom;
	else if(!strcmp("bottom", p->winA)) anchor = bottom;
	else if(!strcmp("bottom_right", p->winA)) { 
		width_margin *= -1; anchor = right|bottom;
	}

	if(!strncmp("bottom", p->winA, 6))
		height_margin *= -1;

	return anchor;
}

void zwlr_init(struct surfaceData *s){
	struct wlOutput *output = wl_output_get_desired();
	struct config_params *p = &s->s->conf;

	// Create a "wallpaper" surface
	xavaWLRLayerSurface = zwlr_layer_shell_v1_get_layer_surface(
		xavaWLRLayerShell, s->surface, output->output,
		ZWLR_LAYER_SHELL_V1_LAYER_BOTTOM, "bottom");

	// adjust position and properties accordingly
	zwlr_layer_surface_v1_set_size(xavaWLRLayerSurface, p->w, p->h);
	zwlr_layer_surface_v1_set_anchor(xavaWLRLayerSurface, 
			handle_window_alignment(&s->s->conf));
	zwlr_layer_surface_v1_set_margin(xavaWLRLayerSurface, 
			height_margin, -width_margin, -height_margin, width_margin);
	zwlr_layer_surface_v1_set_exclusive_zone(xavaWLRLayerSurface, -1);

	// same stuff as xdg_surface_add_listener, but for zwlr_layer_surface
	zwlr_layer_surface_v1_add_listener(xavaWLRLayerSurface,
		&layer_surface_listener, s);
}

void zwlr_cleanup() {
	zwlr_layer_surface_v1_destroy(xavaWLRLayerSurface);
	zwlr_layer_shell_v1_destroy(xavaWLRLayerShell);
}
