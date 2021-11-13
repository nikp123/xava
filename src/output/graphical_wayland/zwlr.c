#include <string.h>

#include "../../shared.h"
#include "../graphical.h"

#include "gen/wlr-layer-shell-unstable-v1-client-protocol.h"
#include "zwlr.h"
#include "main.h"
#include "wl_output.h"
#include "egl.h"

static struct zwlr_layer_surface_v1 *xavaWLRLayerSurface;
struct zwlr_layer_shell_v1 *xavaWLRLayerShell;

struct zwlr_alignment_info {
    int32_t width_margin;
    int32_t height_margin;
    uint32_t anchor;
};

static void layer_surface_configure(void *data,
        struct zwlr_layer_surface_v1 *surface,
        uint32_t serial, uint32_t width, uint32_t height) {
    struct waydata *wd = data;
    struct XAVA_HANDLE *xava = wd->hand;

    if(width != 0 && height != 0) {
        calculate_inner_win_pos(xava, width, height);

        waylandEGLWindowResize(wd, width, height);

        pushXAVAEventStack(wd->events, XAVA_REDRAW);
        pushXAVAEventStack(wd->events, XAVA_RESIZE);
    }

    // Respond to compositor
    zwlr_layer_surface_v1_ack_configure(surface, serial);
}

static void layer_surface_closed(void *data,
        struct zwlr_layer_surface_v1 *surface) {
    //destroy_swaybg_output(output);
    xavaLog("zwlr_layer_surface lost");
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
static struct zwlr_alignment_info handle_window_alignment(struct config_params *p) {
    const uint32_t top = ZWLR_LAYER_SURFACE_V1_ANCHOR_TOP;
    const uint32_t bottom = ZWLR_LAYER_SURFACE_V1_ANCHOR_BOTTOM;
    const uint32_t left = ZWLR_LAYER_SURFACE_V1_ANCHOR_LEFT;
    const uint32_t right = ZWLR_LAYER_SURFACE_V1_ANCHOR_RIGHT;

    struct zwlr_alignment_info align = {0};

    align.width_margin = p->wx;
    align.height_margin = p->wy;

    // margins are reversed on opposite edges for user convenience

    // Top alingments
    if(!strcmp("top", p->winA))
        align.anchor = top;
    else if(!strcmp("top_left", p->winA))
        align.anchor = top|left;
    else if(!strcmp("top_right", p->winA)) {
        align.width_margin *= -1;
        align.anchor = top|right;
    }

    // Middle alignments
    else if(!strcmp("left", p->winA))
        align.anchor = left;
    else if(!strcmp("center", p->winA)) { /** nop **/ }
    else if(!strcmp("right", p->winA)) {
        align.width_margin *= -1;
        align.anchor = right;
    }

    // Bottom alignments
    else if(!strcmp("bottom_left", p->winA))
        align.anchor = left|bottom;
    else if(!strcmp("bottom", p->winA))
        align.anchor = bottom;
    else if(!strcmp("bottom_right", p->winA)) {
        align.width_margin *= -1;
        align.anchor = right|bottom;
    }

    if(!strncmp("bottom", p->winA, 6))
        align.height_margin *= -1;

    return align;
}

void zwlr_init(struct waydata *wd) {
    struct XAVA_HANDLE     *hand   = wd->hand;
    struct config_params   *p      = &hand->conf;
    struct wlOutput        *output = wl_output_get_desired();

    // Create a "wallpaper" surface
    xavaWLRLayerSurface = zwlr_layer_shell_v1_get_layer_surface(
        xavaWLRLayerShell, wd->surface, output->output,
        ZWLR_LAYER_SHELL_V1_LAYER_BOTTOM, "bottom");

    uint32_t width = p->w, height = p->h;
    struct zwlr_alignment_info align = handle_window_alignment(p);
    if(p->fullF) {
        width =  output->width;
        height = output->height;
        align.anchor = 0; // 0 resets the alignment properties... right?
        align.width_margin = 0;
        align.height_margin = 0;
    }

    // adjust position and properties accordingly
    zwlr_layer_surface_v1_set_size(xavaWLRLayerSurface, width, height);
    zwlr_layer_surface_v1_set_anchor(xavaWLRLayerSurface,
            align.anchor);
    zwlr_layer_surface_v1_set_margin(xavaWLRLayerSurface,
            align.height_margin, -align.width_margin,
            -align.height_margin, align.width_margin);
    zwlr_layer_surface_v1_set_exclusive_zone(xavaWLRLayerSurface, -1);

    // same stuff as xdg_surface_add_listener, but for zwlr_layer_surface
    zwlr_layer_surface_v1_add_listener(xavaWLRLayerSurface,
        &layer_surface_listener, wd);
}

void zwlr_cleanup(struct waydata *wd) {
    zwlr_layer_surface_v1_destroy(xavaWLRLayerSurface);
    zwlr_layer_shell_v1_destroy(xavaWLRLayerShell);
}

