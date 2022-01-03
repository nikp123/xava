#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>

#include "shared.h"

#ifdef WAYLAND
bool am_i_wayland(void);
#endif

#ifdef X11
bool am_i_x11(void);
#endif

#ifdef WINDOWS
bool am_i_win32(void);
#endif

#ifdef SDL2
bool am_i_sdl2(void);
#endif

typedef struct system {
    bool (*test_func)(void);

    // if these strings are empty, that means that the system is not supported
    char *cairo;
    char *opengl;
} sys;

sys systems[] = {
#ifdef WAYLAND
    { .test_func = am_i_wayland, .cairo = "wayland_cairo", .opengl = "wayland_opengl" },
#endif
#ifdef WINDOWS
    { .test_func = am_i_win32, .cairo = "win_cairo", .opengl = "win_opengl" },
#endif
#ifdef X11
    { .test_func = am_i_x11, .cairo = "x11_cairo", .opengl = "x11_opengl" },
#endif
#ifdef SDL2
    { .test_func = am_i_sdl2, .cairo = NULL, .opengl = "sdl2_opengl" },
#endif
};

struct functions {
    void     (*cleanup)     (XAVA *xava);
    int      (*init)        (XAVA *xava);
    void     (*clear)       (XAVA *xava);
    int      (*apply)       (XAVA *xava);
    XG_EVENT (*handle_input)(XAVA *xava);
    void     (*draw)        (XAVA *xava);
    void     (*load_config) (XAVA *xava);
} functions;

XAVAMODULE *module;

void xavaOutputLoadConfig(XAVA *xava) {
    int supported_systems = sizeof(systems)/sizeof(sys);

    char *system = NULL;
    int i = 0;

    xava_output_module_default_find_any_remaining:
    for(; i < supported_systems; i++) {
        if(systems[i].test_func()) {
            #ifdef CAIRO
            system = systems[i].cairo;
            if(system != NULL)
                break;
            #endif
            #ifdef OPENGL
            system = systems[i].opengl;
            if(system != NULL)
                break;
            #endif
        }
    }

    xavaBailCondition(system == NULL,
        "No supported output methods found for '%s'",
    #if defined(CAIRO)
        "cairo"
    #elif defined(OPENGL)
        "opengl"
    #else
        #error "Build broke, pls fix!"
        "wtf"
    #endif
        );

    module = xava_module_output_load(system);
    if(!xava_module_valid(module)) {
        // execution halts here if the condition fails btw
        xavaBailCondition(i == supported_systems-1,
            "xava module failed to load (definitely bug): %s",
            xava_module_error_get(module));

        xavaLog("xava module failed to load (probably bug): %s",
            xava_module_error_get(module));
        goto xava_output_module_default_find_any_remaining;
    }


    functions.cleanup      = xava_module_symbol_address_get(module, "xavaOutputCleanup");
    functions.init         = xava_module_symbol_address_get(module, "xavaInitOutput");
    functions.clear        = xava_module_symbol_address_get(module, "xavaOutputClear");
    functions.apply        = xava_module_symbol_address_get(module, "xavaOutputApply");
    functions.handle_input = xava_module_symbol_address_get(module, "xavaOutputHandleInput");
    functions.draw         = xava_module_symbol_address_get(module, "xavaOutputDraw");
    functions.load_config  = xava_module_symbol_address_get(module, "xavaOutputLoadConfig");

    functions.load_config(xava);
}

EXP_FUNC void xavaOutputCleanup(XAVA *xava) {
    functions.cleanup(xava);

    xava_module_free(module);
}

EXP_FUNC int xavaInitOutput(XAVA *xava) {
    return functions.init(xava);
}

EXP_FUNC void xavaOutputClear(XAVA *xava) {
    functions.clear(xava);
}

EXP_FUNC int xavaOutputApply(XAVA *xava) {
    return functions.apply(xava);
}

EXP_FUNC XG_EVENT xavaOutputHandleInput(XAVA *xava) {
    return functions.handle_input(xava);
}

EXP_FUNC void xavaOutputDraw(XAVA *xava) {
    functions.draw(xava);
}

