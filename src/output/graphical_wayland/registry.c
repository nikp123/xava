#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <wayland-client.h>

#include "wl_output.h"
#include "registry.h"
#include "zwlr.h"
#include "main.h"
#include "xdg.h"

struct wl_registry *xavaWLRegistry;

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
		//output->state = state;

		int i = xavaWLOutputsCount;
		struct wlOutput **wlo;
		wlo = reallocarray(xavaWLOutputs, (++xavaWLOutputsCount), sizeof(struct wlOutput*));
		if(wlo == NULL) {
			fprintf(stderr, "Could not allocate xavaWLOutputs!\n");
			exit(EXIT_FAILURE); // no safe exit
		}

		wlo[i] = malloc(sizeof(struct wlOutput));
		wlo[i]->output = wl_registry_bind(xavaWLRegistry, name, &wl_output_interface, 2);
		wlo[i]->name = name;

		wl_output_add_listener(wlo[i]->output, &output_listener, wlo[i]);
		xavaWLOutputs = wlo;

		wl_display_roundtrip(xavaWLDisplay);
	}
}

static void xava_wl_registry_global_remove(void *data, struct wl_registry *wl_registry,
		uint32_t name) {
	// This sometimes happens when displays get reconfigured

	storedEvent = XAVA_RELOAD;

	#ifdef DEBUG
		fprintf(stderr, "wayland: wl_registry died\n");
	#endif
}

const struct wl_registry_listener xava_wl_registry_listener = {
	.global = xava_wl_registry_global_listener,
	.global_remove = xava_wl_registry_global_remove,
};

