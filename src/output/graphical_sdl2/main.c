#include <time.h>

#include <SDL.h>
#include <SDL_opengl.h>
#define GL_ALREADY_DEFINED

#include "../shared/glew.h"

#include "../graphical.h"
#include "../../config.h"
#include "../../shared.h"

SDL_Window *xavaSDLWindow;
SDL_Surface *xavaSDLWindowSurface;
SDL_Event xavaSDLEvent;
SDL_DisplayMode xavaSDLVInfo;

SDL_GLContext xavaSDLGLContext;

EXP_FUNC void xavaOutputCleanup(struct XAVA_HANDLE *s)
{
    GLCleanup(s);
    SDL_GL_DeleteContext(xavaSDLGLContext);
    SDL_FreeSurface(xavaSDLWindowSurface);
    SDL_DestroyWindow(xavaSDLWindow);
    SDL_Quit();
}

EXP_FUNC int xavaInitOutput(struct XAVA_HANDLE *s)
{
    struct config_params *p = &s->conf;

    xavaBailCondition(SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS),
            "Unable to initailize SDL2: %s", SDL_GetError());

    // calculating window x and y position
    xavaBailCondition(SDL_GetCurrentDisplayMode(0, &xavaSDLVInfo),
            "Error opening display: %s", SDL_GetError());
    calculate_win_pos(p, xavaSDLVInfo.w, xavaSDLVInfo.h);

    // creating a window
    Uint32 windowFlags = SDL_WINDOW_RESIZABLE | SDL_WINDOW_OPENGL;
    if(p->fullF) windowFlags |= SDL_WINDOW_FULLSCREEN;
    if(!p->borderF) windowFlags |= SDL_WINDOW_BORDERLESS;
    if(p->vsync) windowFlags |= SDL_RENDERER_PRESENTVSYNC;
    xavaSDLWindow = SDL_CreateWindow("XAVA", p->wx, p->wy, p->w, p->h, windowFlags);
    xavaBailCondition(!xavaSDLWindow, "SDL window cannot be created: %s", SDL_GetError());
    //SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "ERROR", "cannot create SDL window", NULL);

    xavaSDLGLContext = SDL_GL_CreateContext(xavaSDLWindow);
    GLInit(s);

    return 0;
}

EXP_FUNC void xavaOutputClear(struct XAVA_HANDLE *s) {
    GLClear(s);
}

EXP_FUNC int xavaOutputApply(struct XAVA_HANDLE *s) {
    struct config_params *p = &s->conf;

    // toggle fullscreen
    SDL_SetWindowFullscreen(xavaSDLWindow, SDL_WINDOW_FULLSCREEN & p->fullF);

    xavaSDLWindowSurface = SDL_GetWindowSurface(xavaSDLWindow);
    // Appearently SDL uses multithreading so this avoids invalid access
    // If I had a job, here's what I would be fired for xD
    SDL_Delay(100);
    xavaOutputClear(s);

    GLApply(s);

    // Window size patch, because xava wipes w and h for some reason.
    calculate_inner_win_pos(s, xavaSDLWindowSurface->w, xavaSDLWindowSurface->h);
    return 0;
}

EXP_FUNC XG_EVENT xavaOutputHandleInput(struct XAVA_HANDLE *s) {
    struct config_params *p = &s->conf;

    while(SDL_PollEvent(&xavaSDLEvent) != 0) {
        switch(xavaSDLEvent.type) {
            case SDL_KEYDOWN:
                switch(xavaSDLEvent.key.keysym.sym)
                {
                    // should_reload = 1
                    // resizeTerminal = 2
                    // bail = -1
                    case SDLK_a:
                        p->bs++;
                        return XAVA_RESIZE;
                    case SDLK_s:
                        if(p->bs > 0) p->bs--;
                        return XAVA_RESIZE;
                    case SDLK_ESCAPE:
                        return XAVA_QUIT;
                    case SDLK_f: // fullscreen
                        p->fullF = !p->fullF;
                        return XAVA_RESIZE;
                    case SDLK_UP: // key up
                        p->sens *= 1.05;
                        break;
                    case SDLK_DOWN: // key down
                        p->sens *= 0.95;
                        break;
                    case SDLK_LEFT: // key left
                        p->bw++;
                        return XAVA_RESIZE;
                    case SDLK_RIGHT: // key right
                        if(p->bw > 1) p->bw--;
                        return XAVA_RESIZE;
                    case SDLK_r: // reload config
                        return XAVA_RELOAD;
                    case SDLK_c: // change foreground color
                        if(p->gradients) break;
                        p->col = rand() % 0x100;
                        p->col = p->col << 16;
                        p->col += (unsigned int)rand();
                        return XAVA_REDRAW;
                    case SDLK_b: // change background color
                        p->bgcol = rand() % 0x100;
                        p->bgcol = p->bgcol << 16;
                        p->bgcol += (unsigned int)rand();
                        return XAVA_REDRAW;
                    case SDLK_q:
                        return XAVA_QUIT;
                }
                break;
            case SDL_WINDOWEVENT:
                if(xavaSDLEvent.window.event == SDL_WINDOWEVENT_CLOSE) return -1;
                else if(xavaSDLEvent.window.event == SDL_WINDOWEVENT_RESIZED){
                    // if the user resized the window
                    calculate_inner_win_pos(s, xavaSDLEvent.window.data1, xavaSDLEvent.window.data2);
                    return XAVA_RESIZE;
                }
                break;
        }
    }

    if(GLEvent(s) == XAVA_RELOAD)
        return XAVA_RELOAD;

    return XAVA_IGNORE;
}

EXP_FUNC void xavaOutputDraw(struct XAVA_HANDLE *s) {
    GLDraw(s);
    SDL_GL_SwapWindow(xavaSDLWindow);
    return;
}

EXP_FUNC void xavaOutputLoadConfig(struct XAVA_HANDLE *s) {
    struct config_params *p = &s->conf;
    //struct XAVA_CONFIG config = s->default_config.config;

    // VSync doesnt work on SDL2 :(
    p->vsync = 0;

    GLConfigLoad(s);
}

