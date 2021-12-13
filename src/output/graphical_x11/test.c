#include <stdbool.h>
#include <X11/X.h>
#include <X11/Xlib.h>

bool am_i_x11(void) {
    Display *display = XOpenDisplay(NULL);
    if(display == NULL)
        return false;

    XCloseDisplay(display);
    return true;
}

