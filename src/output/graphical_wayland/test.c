#include <stdbool.h>
#include <wayland-client-core.h>

bool am_i_wayland(void) {
    struct wl_display *display = wl_display_connect(NULL);
    if(display == NULL)
        return false;

    wl_display_disconnect(display);
    return true;
}
