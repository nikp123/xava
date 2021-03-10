#include "render.h"

int xavaWLSHMFD;
_Bool xavaWLCurrentlyDrawing = 0;

uint32_t extern wayland_color_blend(uint32_t color, uint16_t alpha) {
	uint8_t red =   ((color >> 16 & 0xff) | (color >> 8 & 0xff00)) * alpha / 0xffff;
	uint8_t green = ((color >>  8 & 0xff) | (color >> 0 & 0xff00)) * alpha / 0xffff;
	uint8_t blue =  ((color >>  0 & 0xff) | (color << 8 & 0xff00)) * alpha / 0xffff;
	return alpha<<24|red<<16|green<<8|blue;
}

static void wl_buffer_release(void *data, struct wl_buffer *wl_buffer) {
	/* Sent by the compositor when it's no longer using this buffer */
	wl_buffer_destroy(wl_buffer);
}
static const struct wl_buffer_listener wl_buffer_listener = {
	.release = wl_buffer_release,
};

struct wl_buffer *wl_create_framebuffer(struct waydata *wd) {
	struct config_params *p = &wd->s->conf;

	int width = p->w, height = p->h;
	int stride = width*4;
	int size = stride * height;

	struct wl_shm_pool *pool = wl_shm_create_pool(wd->shm, xavaWLSHMFD, size);
	struct wl_buffer *buffer = wl_shm_pool_create_buffer(pool,
			0, width, height, stride, WL_SHM_FORMAT_ARGB8888);
	wl_shm_pool_destroy(pool);

	wl_buffer_add_listener(buffer, &wl_buffer_listener, NULL);
	return buffer;
}

void update_frame(struct waydata *wd) {
	struct config_params *p = &wd->s->conf;

	// Vsync kind of thing here
	while(xavaWLCurrentlyDrawing) usleep(1000);

	int size = p->w*p->h*sizeof(uint32_t);
	ftruncate(xavaWLSHMFD, size);

	// Update frame and inform wayland 
	struct wl_buffer *buffer = wl_create_framebuffer(wd);
	wl_surface_attach(wd->surface, buffer, 0, 0);
	//wl_surface_damage_buffer(xavaWLSurface, 0, 0, INT32_MAX, INT32_MAX);
	wl_surface_commit(wd->surface);
}


