#include "gen/xdg-shell-client-protocol.h"

#include "render.h"
#include "xdg.h"

static struct xdg_surface *xavaXDGSurface;
static struct xdg_toplevel *xavaXDGToplevel;

struct xdg_wm_base *xavaXDGWMBase;

static void xdg_wm_base_ping(void *data, struct xdg_wm_base *xdg_wm_base,
		uint32_t serial) {
	xdg_wm_base_pong(xdg_wm_base, serial);
}

const struct xdg_wm_base_listener xdg_wm_base_listener = {
	.ping = xdg_wm_base_ping,
};

static void xdg_toplevel_handle_configure(void *data,
		struct xdg_toplevel *xdg_toplevel, int32_t w, int32_t h,
		struct wl_array *states) {
	struct waydata *s = data;

	if (w == 0 || h == 0) return;

	s->s->conf.w = w;
	s->s->conf.h = h;
	s->event = XAVA_RESIZE;
}

static void xdg_toplevel_handle_close(void *data,
		struct xdg_toplevel *xdg_toplevel) {
	struct waydata *s = data;

	s->event = XAVA_QUIT;
}

struct xdg_toplevel_listener xdg_toplevel_listener = {
	.configure = xdg_toplevel_handle_configure,
	.close = xdg_toplevel_handle_close,
};

static void xdg_surface_configure(void *data, struct xdg_surface *xdg_surface,
		uint32_t serial) {
	struct waydata *s = data;

	// confirm that you exist to the compositor
	xdg_surface_ack_configure(xdg_surface, serial);

	update_frame(s);
}

const struct xdg_surface_listener xdg_surface_listener = {
	.configure = xdg_surface_configure,
};

void xdg_init(struct waydata *s) {
	// create window, or "surface" in waland terms
	xavaXDGSurface = xdg_wm_base_get_xdg_surface(xavaXDGWMBase, s->surface);

	// for those unaware, the compositor baby sits everything that you
	// make, thus it needs a function through which the compositor
	// will manage your application
	xdg_surface_add_listener(xavaXDGSurface, &xdg_surface_listener, s);

	xavaXDGToplevel = xdg_surface_get_toplevel(xavaXDGSurface);
	xdg_toplevel_set_title(xavaXDGToplevel, "XAVA");
	xdg_toplevel_add_listener(xavaXDGToplevel, &xdg_toplevel_listener, s);
}

void xdg_cleanup() {
	xdg_toplevel_destroy(xavaXDGToplevel);
	xdg_surface_destroy(xavaXDGSurface);
}