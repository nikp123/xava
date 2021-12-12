#include <string.h>
#include <unistd.h>
#include <sys/mman.h>
#include <errno.h>

#ifndef SHM
    #define SHM
#endif

#include "shm.h"
#include "main.h"

uint32_t wayland_color_blend(uint32_t color, uint8_t alpha) {
    uint8_t red =   ((color >> 16 & 0xff) | (color >> 8 & 0xff00)) * alpha / 0xff;
    uint8_t green = ((color >>  8 & 0xff) | (color >> 0 & 0xff00)) * alpha / 0xff;
    uint8_t blue =  ((color >>  0 & 0xff) | (color << 8 & 0xff00)) * alpha / 0xff;
    return alpha<<24|red<<16|green<<8|blue;
}

void wl_buffer_release(void *data, struct wl_buffer *wl_buffer) {
    //struct waydata *wd = data;
    /* Sent by the compositor when it's no longer using this buffer */
    wl_buffer_destroy(wl_buffer);
}
const struct wl_buffer_listener wl_buffer_listener = {
    .release = wl_buffer_release,
};

struct wl_buffer *wl_create_framebuffer(struct waydata *wd) {
    XAVA   *xava = wd->hand;

    int width = xava->outer.w, height = xava->outer.h;
    int stride = width*sizeof(uint32_t);
    int size = stride * height;

    struct wl_shm_pool *pool = wl_shm_create_pool(wd->shm.ref, wd->shm.fd, size);
    struct wl_buffer *buffer = wl_shm_pool_create_buffer(pool,
            0, width, height, stride, WL_SHM_FORMAT_ARGB8888);
    wl_shm_pool_destroy(pool);

    wd->shm.dim.w      = width;
    wd->shm.dim.h      = height;
    wd->shm.dim.stride = stride;

    wl_buffer_add_listener(buffer, &wl_buffer_listener, wd);

    return buffer;
}

void update_frame(struct waydata *wd) {
    //XAVA_CONFIG     *p    = &wd->s->conf;

    // Update frame and inform wayland
    struct wl_buffer *buffer = wl_create_framebuffer(wd);
    wl_surface_attach(wd->surface, buffer, 0, 0);
    //wl_surface_damage_buffer(xavaWLSurface, 0, 0, INT32_MAX, INT32_MAX);
    wl_surface_commit(wd->surface);
}

void reallocSHM(struct waydata *wd) {
    XAVA   *hand = wd->hand;

    munmap(wd->shm.buffer, wd->shm.max_size);

    int size = hand->outer.w *
               hand->outer.h *
               sizeof(uint32_t);
    if(size > wd->shm.max_size) {
        wd->shm.max_size = size;
        xavaErrorCondition(ftruncate(wd->shm.fd, size) == -1,
                "%s", strerror(errno));
    }

    wd->shm.dim.w      = hand->outer.w;
    wd->shm.dim.h      = hand->outer.h;
    wd->shm.dim.stride = hand->outer.w*sizeof(uint32_t);

    wd->shm.buffer = mmap(NULL,
            wd->shm.max_size,
            PROT_READ | PROT_WRITE,
            MAP_SHARED,
            wd->shm.fd,
            0);

    xavaBailCondition(wd->shm.buffer == MAP_FAILED,
            "%s", strerror(errno));

    wl_surface_commit(wd->surface);
}

void closeSHM(struct waydata *wd) {
    close(wd->shm.fd);
    munmap(wd->shm.buffer, wd->shm.dim.stride*wd->shm.dim.h);

    wd->shm.fd = -1;
    wd->shm.buffer = NULL;
}

