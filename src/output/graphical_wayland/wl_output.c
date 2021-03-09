#include <stdlib.h>

#include "wl_output.h"
#include "main.h"

struct xdg_output_manager *xavaXDGOutputManager;

struct wlOutput **xavaWLOutputs;
int xavaWLOutputsCount;

static void output_geometry(void *data, struct wl_output *output, int32_t x,
		int32_t y, int32_t width_mm, int32_t height_mm, int32_t subpixel,
		const char *make, const char *model, int32_t transform) {
	// Who cares
}

static void output_mode(void *data, struct wl_output *output, uint32_t flags,
		int32_t width, int32_t height, int32_t refresh) {
	// Who cares
}

static void output_done(void *data, struct wl_output *output) {
	// Who cares
}

static void output_scale(void *data, struct wl_output *wl_output,
		int32_t scale) {
	struct wlOutput *output = data;
	output->scale = scale;
}

const struct wl_output_listener output_listener = {
	.geometry = output_geometry,
	.mode = output_mode,
	.done = output_done,
	.scale = output_scale,
};

void wl_output_cleanup() {
	for(int i = 0; i < xavaWLOutputsCount; i++) {
		wl_output_destroy(xavaWLOutputs[i]->output);
		free(xavaWLOutputs[i]);
	} xavaWLOutputsCount = 0;
	free(xavaWLOutputs);
}

struct wlOutput *wl_output_get_desired() {
	// select an appropriate output
	return xavaWLOutputs[monitorNumber];
}

