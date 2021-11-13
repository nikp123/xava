#include <time.h>

#include <SDL.h>

#include "../../graphical.h"
#include "../../../config.h"
#include "../../../shared.h"

SDL_Window *xavaSDLWindow;
SDL_Surface *xavaSDLWindowSurface;
SDL_Renderer *xavaSDLWindowRenderer;
SDL_Event xavaSDLEvent;
SDL_DisplayMode xavaSDLVInfo;
int *gradCol;

EXP_FUNC void xavaOutputCleanup(struct XAVA_HANDLE *s)
{
    free(gradCol);
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
    Uint32 windowFlags = SDL_WINDOW_RESIZABLE;
    if(p->fullF) windowFlags |= SDL_WINDOW_FULLSCREEN;
    if(!p->borderF) windowFlags |= SDL_WINDOW_BORDERLESS;
    if(p->vsync) windowFlags |= SDL_RENDERER_PRESENTVSYNC;
    SDL_CreateWindowAndRenderer(p->w, p->h, windowFlags, &xavaSDLWindow, &xavaSDLWindowRenderer);
    //SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "ERROR", "cannot create SDL window", NULL);


    if(p->gradients) {
        gradCol = malloc(sizeof(int)*p->gradients);
        for(unsigned int i=0; i<p->gradients; i++)
            sscanf(p->gradient_colors[i], "#%x", &gradCol[i]);
    }

    return 0;
}

EXP_FUNC void xavaOutputClear(struct XAVA_HANDLE *s) {
    struct config_params *p = &s->conf;
    SDL_FillRect(xavaSDLWindowSurface, NULL, SDL_MapRGB(xavaSDLWindowSurface->format, p->bgcol/0x10000%0x100, p->bgcol/0x100%0x100, p->bgcol%0x100));
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

    // Window size patch, because xava wipes w and h for some reason.
    p->w = xavaSDLWindowSurface->w;
    p->h = xavaSDLWindowSurface->h;

    SDL_SetRenderDrawBlendMode(xavaSDLWindowRenderer, SDL_BLENDMODE_BLEND);
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
                    p->w = xavaSDLEvent.window.data1;
                    p->h = xavaSDLEvent.window.data2;
                    return XAVA_RESIZE;
                }
                break;
        }
    }
    return XAVA_IGNORE;
}

EXP_FUNC void xavaOutputDraw(struct XAVA_HANDLE *s) {
    struct config_params *p = &s->conf;

    struct audio_data *audio = &s->audio;

    SDL_SetRenderDrawColor(xavaSDLWindowRenderer, ARGB_R_32(p->bgcol),
            ARGB_G_32(p->bgcol), ARGB_B_32(p->bgcol), SDL_ALPHA_OPAQUE);
    SDL_RenderClear(xavaSDLWindowRenderer);

    int offsetx, offsety, size;
    if(p->w > p->h) {
        size = p->h;
        offsetx = p->w-p->h;
        offsetx /= 2;
        offsety = 0;
    } else {
        size = p->w;
        offsety = p->h-p->w;
        offsety /= 2;
        offsetx = 0;
    }

    for(int i = 1; i < audio->inputsize; i++) {
        int x1, y1, x2, y2;
        x1 = offsetx+(uint32_t)size*(audio->audio_out_l[i-1]+32768)/65536;
        y1 = offsety+(uint32_t)size*(-audio->audio_out_r[i-1]+32768)/65536;
        x2 = offsetx+(uint32_t)size*(audio->audio_out_l[i]+32768)/65536;
        y2 = offsety+(uint32_t)size*(-audio->audio_out_r[i]+32768)/65536;

        float distance = hypot(abs(x1-x2), abs(y1-y2));;
        distance += 1.0;
        SDL_SetRenderDrawColor(xavaSDLWindowRenderer, ARGB_R_32(p->col),
                ARGB_G_32(p->col), ARGB_B_32(p->col), MIN(255, 1024/distance));
        //xavaLog("(%d,%d) -> (%d,%d)", x1, y1, x2, y2);
        SDL_RenderDrawLine(xavaSDLWindowRenderer, x1, y1, x2, y2);
    }

    SDL_RenderPresent(xavaSDLWindowRenderer);
    return;
}

EXP_FUNC void xavaOutputLoadConfig(struct XAVA_HANDLE *hand) {
    struct config_params *p = &hand->conf;

    // VSync doesnt work on SDL2 :(
    p->vsync = 0;

    // because we don't want neither FFT or mono in osciloscope mode
    p->skipFilterF = true;
    p->stereo = true;

    // feel free to change this if you'd like
    p->inputsize = p->samplerate / p->framerate;

    // change some default settings for this mode specifically
    p->w         = xavaConfigGetInt(hand->default_config.config, "window", "width", 800);
    p->h         = xavaConfigGetInt(hand->default_config.config, "window", "height", 600);
    p->fullF     = xavaConfigGetBool(hand->default_config.config, "window", "fullscreen", 0);
    p->transF    = xavaConfigGetBool(hand->default_config.config, "window", "transparency", 0);
    p->borderF   = xavaConfigGetBool(hand->default_config.config, "window", "border", 1);
    p->bottomF   = xavaConfigGetBool(hand->default_config.config, "window", "keep_below", 0);
    p->interactF = xavaConfigGetBool(hand->default_config.config, "window", "interactable", 1);
    p->taskbarF  = xavaConfigGetBool(hand->default_config.config, "window", "taskbar_icon", 1);
    p->holdSizeF = xavaConfigGetBool(hand->default_config.config, "window", "hold_size", false);
}
