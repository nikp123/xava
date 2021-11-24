#ifndef __WAYLAND_EGL_H
#define __WAYLAND_EGL_H
    #include <stdlib.h>
    #include "main.h"

    void     waylandEGLWindowResize(struct waydata *wd, int w, int h);
    void     waylandEGLCreateWindow(struct waydata *wd);
#endif
