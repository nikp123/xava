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
    struct waydata *wd = data;

    UNUSED(version);

    #ifdef SHM
        if (strcmp(interface, wl_shm_interface.name) == 0) {
            wd->shm.ref = wl_registry_bind(
                    wl_registry, name, &wl_shm_interface, 1);
        } else
    #endif
    if (strcmp(interface, wl_compositor_interface.name) == 0) {
        wd->compositor = wl_registry_bind(
            wl_registry, name, &wl_compositor_interface, 4);
    } else if (strcmp(interface, xdg_wm_base_interface.name) == 0) {
        xavaXDGWMBase = wl_registry_bind(
            wl_registry, name, &xdg_wm_base_interface, 1);
        xdg_wm_base_add_listener(xavaXDGWMBase,
            &xdg_wm_base_listener, NULL);
    } else if (strcmp(interface, zwlr_layer_shell_v1_interface.name) == 0) {
        xavaWLRLayerShell = wl_registry_bind(xavaWLRegistry, name,
                &zwlr_layer_shell_v1_interface, 1);
    } else if (strcmp(interface, zxdg_output_manager_v1_interface.name) == 0) {
        xavaXDGOutputManager = wl_registry_bind(xavaWLRegistry, name,
            &zxdg_output_manager_v1_interface, 2);
    } else if (strcmp(interface, wl_output_interface.name) == 0) {
        if(!xavaXDGOutputManager)
            return;

        struct wlOutput *output = malloc(sizeof(struct wlOutput));
        output->output = wl_registry_bind(xavaWLRegistry, name,
                &wl_output_interface, 3);
        output->id     = name;

        wl_output_add_listener(output->output, &output_listener, output);
        wl_list_insert(&wd->outputs, &output->link);

        output->xdg_output = zxdg_output_manager_v1_get_xdg_output(
            xavaXDGOutputManager, output->output);
        zxdg_output_v1_add_listener(output->xdg_output,
            &xdg_output_listener, output);
    }
}

static void xava_wl_registry_global_remove(void *data, struct wl_registry *wl_registry,
        uint32_t name) {
    UNUSED(wl_registry);
    UNUSED(name);

    struct waydata           *wd   = data;

    // This sometimes happens when displays get reconfigured
    pushXAVAEventStack(wd->events, XAVA_RELOAD);

    xavaLog("wl_registry died");
}

const struct wl_registry_listener xava_wl_registry_listener = {
    .global = xava_wl_registry_global_listener,
    .global_remove = xava_wl_registry_global_remove,
};

