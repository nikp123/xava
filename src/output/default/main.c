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

EXP_FUNC void xavaOutputLoadConfig(XAVA *xava) {
    int supported_systems = sizeof(systems)/sizeof(sys);

    char *system = NULL;

    for(int i = 0; i < supported_systems; i++) {
        // First test which graphics/output mode works
        // (depends on which are compiled in)
        if(systems[i].test_func()) {
            /**
             * In hindsight, I should've left a comment but basically
             * these if cases do execute separately. Hence one exiting
             * skipping the loop cycle here early is fine.
             *
             * That's because these code branches execute separately,
             * ie. only one of them gets compiled into the resulting binary.
             **/
            #ifdef CAIRO
            system = systems[i].cairo;
            if(system == NULL)
                continue;
            #endif

            #ifdef OPENGL
            system = systems[i].opengl;
            if(system == NULL)
                continue;
            #endif
        }

        module = xava_module_output_load(system);

        // only legitimate way to exit the loading loop
        if(xava_module_valid(module))
            break;

        xavaLog("xava module failed to load (probably bug): %s",
            xava_module_error_get(module));
    }

    // After we've exhausted all the options we inform the user that we have
    // failed and thus crash the program.
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

    // From here onwards, we don't need to iterate because we've succeeded
    // in loading so far.
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

