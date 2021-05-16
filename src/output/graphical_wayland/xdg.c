#include "gen/xdg-shell-client-protocol.h"

#include "render.h"
#include "xdg.h"
#include "main.h"
#include <wayland-client-core.h>

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
	struct waydata           *wd   = data;
	struct XAVA_HANDLE       *hand = wd->hand;
	struct config_params     *p    = &hand->conf;

	if(w == 0 && h == 0) return;

	if(p->w != w && p->h != h) {
		#ifndef EGL
			while(wd->fbUnsafe)
				usleep(10);

			wd->fbUnsafe = true;
		#endif

		p->w = w;
		p->h = h;

		#ifdef EGL
			wl_egl_window_resize(wd->ESContext.native_window, w, h, 0, 0);
			wl_surface_commit(wd->surface);
		#else
			reallocSHM(wd);
			wd->fbUnsafe = false;
		#endif

		pushXAVAEventStack(wd->events, XAVA_REDRAW);
		pushXAVAEventStack(wd->events, XAVA_RESIZE);
	}
}

static void xdg_toplevel_handle_close(void *data,
		struct xdg_toplevel *xdg_toplevel) {
	struct waydata           *wd   = data;
	struct XAVA_HANDLE       *hand = wd->hand;

	pushXAVAEventStack(wd->events, XAVA_QUIT);
}

struct xdg_toplevel_listener xdg_toplevel_listener = {
	.configure = xdg_toplevel_handle_configure,
	.close = xdg_toplevel_handle_close,
};

static void xdg_surface_configure(void *data, struct xdg_surface *xdg_surface,
		uint32_t serial) {
	struct waydata           *wd   = data;
	//struct function_pointers *func = &wd->s->func; 

	// confirm that you exist to the compositor
	xdg_surface_ack_configure(xdg_surface, serial);

	#ifndef EGL
		update_frame(wd);
	#endif
}

const struct xdg_surface_listener xdg_surface_listener = {
	.configure = xdg_surface_configure,
};

void xdg_init(struct waydata *wd) {
	// create window, or "surface" in waland terms
	xavaXDGSurface = xdg_wm_base_get_xdg_surface(xavaXDGWMBase, wd->surface);

	// for those unaware, the compositor baby sits everything that you
	// make, thus it needs a function through which the compositor
	// will manage your application
	xdg_surface_add_listener(xavaXDGSurface, &xdg_surface_listener, wd);

	xavaXDGToplevel = xdg_surface_get_toplevel(xavaXDGSurface);
	xdg_toplevel_set_title(xavaXDGToplevel, "XAVA");
	xdg_toplevel_add_listener(xavaXDGToplevel, &xdg_toplevel_listener, wd);
}

void xdg_cleanup() {
	xdg_toplevel_destroy(xavaXDGToplevel);
	xdg_surface_destroy(xavaXDGSurface);
}
