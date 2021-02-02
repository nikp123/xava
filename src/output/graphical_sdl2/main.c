#include <SDL.h>
#include <time.h>
#include "../graphical.h"
#include "../../config.h"
#include "../../shared.h"

SDL_Window *xavaSDLWindow;
SDL_Surface *xavaSDLWindowSurface;
SDL_Event xavaSDLEvent;
SDL_DisplayMode xavaSDLVInfo;
int *gradCol;

void xavaOutputCleanup(void)
{
	free(gradCol);
	SDL_FreeSurface(xavaSDLWindowSurface);
	SDL_DestroyWindow(xavaSDLWindow);
	SDL_Quit();
}

int xavaInitOutput()
{
	if(SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS) != 0)
	{
		fprintf(stderr, "unable to initilize SDL2: %s\n", SDL_GetError());
		return 1;
	}
	// calculating window x and y position
	if(SDL_GetCurrentDisplayMode(0, &xavaSDLVInfo)){
		fprintf(stderr, "Error opening display! %s\n", SDL_GetError());
		return 1;
	}
	calculate_win_pos(&(p.wx), &(p.wy), p.w, p.h, xavaSDLVInfo.w, xavaSDLVInfo.h, p.winA);

	// creating a window
	Uint32 windowFlags = SDL_WINDOW_RESIZABLE;
	if(p.fullF) windowFlags |= SDL_WINDOW_FULLSCREEN;
	if(!p.borderF) windowFlags |= SDL_WINDOW_BORDERLESS;
	if(p.vsync) windowFlags |= SDL_RENDERER_PRESENTVSYNC;
	xavaSDLWindow = SDL_CreateWindow("XAVA", p.wx, p.wy, p.w, p.h, windowFlags);
	if(!xavaSDLWindow)
	{
		fprintf(stderr, "SDL window cannot be created: %s\n", SDL_GetError());
		SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "ERROR", "cannot create SDL window", NULL);
		SDL_Quit();
		return 1;
	}

	if(p.gradients) {
		gradCol = malloc(sizeof(int)*p.gradients);
		for(unsigned int i=0; i<p.gradients; i++)
			sscanf(p.gradient_colors[i], "#%x", &gradCol[i]);
	}
	return 0;
}

void xavaOutputClear(void) {
	SDL_FillRect(xavaSDLWindowSurface, NULL, SDL_MapRGB(xavaSDLWindowSurface->format, p.bgcol/0x10000%0x100, p.bgcol/0x100%0x100, p.bgcol%0x100));
}

int xavaOutputApply(void) {
	// toggle fullscreen
	SDL_SetWindowFullscreen(xavaSDLWindow, SDL_WINDOW_FULLSCREEN & p.fullF);

	xavaSDLWindowSurface = SDL_GetWindowSurface(xavaSDLWindow);
	// Appearently SDL uses multithreading so this avoids invalid access
	// If I had a job, here's what I would be fired for xD
	SDL_Delay(100);
	xavaOutputClear();

	// Window size patch, because xava wipes w and h for some reason.
	p.w = xavaSDLWindowSurface->w;
	p.h = xavaSDLWindowSurface->h;
	return 0;
}

int xavaOutputHandleInput() {
	while(SDL_PollEvent(&xavaSDLEvent) != 0) {
		switch(xavaSDLEvent.type) {
			case SDL_KEYDOWN:
				switch(xavaSDLEvent.key.keysym.sym)
				{
					// should_reload = 1
					// resizeTerminal = 2
					// bail = -1
					case SDLK_a:
						p.bs++;
						return 2;
					case SDLK_s:
						if(p.bs > 0) p.bs--;
						return 2;
					case SDLK_ESCAPE:
						return -1;
					case SDLK_f: // fullscreen
						p.fullF = !p.fullF;
						return 2;
					case SDLK_UP: // key up
						p.sens *= 1.05;
						break;
					case SDLK_DOWN: // key down
						p.sens *= 0.95;
						break;
					case SDLK_LEFT: // key left
						p.bw++;
						return 2;
					case SDLK_RIGHT: // key right
						if(p.bw > 1) p.bw--;
						return 2;
					case SDLK_r: // reload config
						return 1;
					case SDLK_c: // change foreground color
						if(p.gradients) break;
						p.col = rand() % 0x100;
						p.col = p.col << 16;
						p.col += (unsigned int)rand();
						return 3;
					case SDLK_b: // change background color
						p.bgcol = rand() % 0x100;
						p.bgcol = p.bgcol << 16;
						p.bgcol += (unsigned int)rand();
						return 3;
					case SDLK_q:
						return -1;
				}
				break;
			case SDL_WINDOWEVENT:
				if(xavaSDLEvent.window.event == SDL_WINDOWEVENT_CLOSE) return -1;
				else if(xavaSDLEvent.window.event == SDL_WINDOWEVENT_RESIZED){
					// if the user resized the window
					p.w = xavaSDLEvent.window.data1;
					p.h = xavaSDLEvent.window.data2;
					return 2;
				}
				break;
		}
	}
	return 0;
}
 
void xavaOutputDraw(int bars, int rest, int *f, int *flastd) {
	for(int i = 0; i < bars; i++) {
		SDL_Rect current_bar;
		if(f[i] > flastd[i]){ 
			if(!p.gradients) {
				current_bar = (SDL_Rect) {rest + i*(p.bs+p.bw), p.h - f[i], p.bw, f[i] - flastd[i]};
				SDL_FillRect(xavaSDLWindowSurface, &current_bar, SDL_MapRGB(xavaSDLWindowSurface->format, p.col/0x10000%0x100, p.col/0x100%0x100, p.col%0x100));
			} else {
				for(unsigned int I = (unsigned int)flastd[i]; I < (unsigned int)f[i]; I++) {
					Uint32 color = 0x0;
					double step = (double)(I%((unsigned int)p.h/(p.gradients-1)))/(double)((double)p.h/(p.gradients-1));

					unsigned int gcPhase = (p.gradients-1)*I/(unsigned int)p.h;
					color |= R_ARGB_32(UNSIGNED_TRANS(ARGB_R_32(gradCol[gcPhase]), ARGB_R_32(gradCol[gcPhase+1]), step));
					color |= G_ARGB_32(UNSIGNED_TRANS(ARGB_G_32(gradCol[gcPhase]), ARGB_G_32(gradCol[gcPhase+1]), step));
					color |= B_ARGB_32(UNSIGNED_TRANS(ARGB_B_32(gradCol[gcPhase]), ARGB_B_32(gradCol[gcPhase+1]), step));

					current_bar = (SDL_Rect) {rest + i*(p.bs+p.bw), p.h - (int)I, p.bw, 1};
					SDL_FillRect(xavaSDLWindowSurface, &current_bar, SDL_MapRGB(xavaSDLWindowSurface->format, color/0x10000%0x100, color/0x100%0x100, color%0x100));
				}
			}
		} else if(f[i] < flastd[i]) {
			current_bar = (SDL_Rect) {rest + i*(p.bs+p.bw), p.h - flastd[i], p.bw, flastd[i] - f[i]};
			SDL_FillRect(xavaSDLWindowSurface, &current_bar, SDL_MapRGB(xavaSDLWindowSurface->format, p.bgcol/0x10000%0x100, p.bgcol/0x100%0x100, p.bgcol%0x100));
		}
	}
	SDL_UpdateWindowSurface(xavaSDLWindow);
	return;
}

void xavaOutputHandleConfiguration(void *data) {
	//dictionary *ini = (dictionary*) data;

	// VSync doesnt work on SDL2 :(
	p.vsync = 0;
}

