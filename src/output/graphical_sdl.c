#include <SDL2/SDL.h>
#include <time.h>
#include "graphical.h"
#include "../config.h"

SDL_Window *xavaSDLWindow;
SDL_Surface *xavaSDLWindowSurface;
SDL_Event xavaSDLEvent;
SDL_DisplayMode xavaSDLVInfo;
int *gradCol;

void cleanup_graphical_sdl(void)
{
	free(gradCol);
	SDL_FreeSurface(xavaSDLWindowSurface);
	SDL_DestroyWindow(xavaSDLWindow);
	SDL_Quit();
}

int init_window_sdl()
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
	if(!strcmp(windowAlignment, "top")){
		windowX = (xavaSDLVInfo.w - p.w) / 2 + windowX;
	}else if(!strcmp(windowAlignment, "bottom")){
		windowX = (xavaSDLVInfo.w - p.w) / 2 + windowX;
		windowY = (xavaSDLVInfo.h - p.h) + (-1*windowY);
	}else if(!strcmp(windowAlignment, "top_left")){
		// Nothing to do here :P
	}else if(!strcmp(windowAlignment, "top_right")){
		windowX = (xavaSDLVInfo.w - p.w) + (-1*windowX);
	}else if(!strcmp(windowAlignment, "left")){
		windowY = (xavaSDLVInfo.h - p.h) / 2;
	}else if(!strcmp(windowAlignment, "right")){
		windowX = (xavaSDLVInfo.w - p.w) + (-1*windowX);
		windowY = (xavaSDLVInfo.h - p.h) / 2 + windowY;
	}else if(!strcmp(windowAlignment, "bottom_left")){
		windowY = (xavaSDLVInfo.h - p.h) + (-1*windowY);
	}else if(!strcmp(windowAlignment, "bottom_right")){
		windowX = (xavaSDLVInfo.w - p.w) + (-1*windowX);
		windowY = (xavaSDLVInfo.h - p.h) + (-1*windowY);
	}else if(!strcmp(windowAlignment, "center")){
		windowX = (xavaSDLVInfo.w - p.w) / 2 + windowX;
		windowY = (xavaSDLVInfo.h - p.h) / 2 + windowY;
	}
	// creating a window
	Uint32 windowFlags = SDL_WINDOW_RESIZABLE;
	if(fs) windowFlags |= SDL_WINDOW_FULLSCREEN;
	if(!borderFlag) windowFlags |= SDL_WINDOW_BORDERLESS;
	xavaSDLWindow = SDL_CreateWindow("XAVA", windowX, windowY, p.w, p.h, windowFlags);
	if(!xavaSDLWindow)
	{
		fprintf(stderr, "SDL window cannot be created: %s\n", SDL_GetError());
		SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "ERROR", "cannot create SDL window", NULL);
		SDL_Quit();
		return 1;
	}

	if(p.color[0] != '#') p.col = definedColors[p.col];
	else sscanf(p.color, "#%x", &p.col);
	if(p.bcolor[0] != '#') p.bgcol = definedColors[p.bgcol];
	else sscanf(p.bcolor, "#%x", &p.bgcol);

	if(p.gradient) {
		gradCol = malloc(sizeof(int)*p.gradient_count);
		for(unsigned int i=0; i<p.gradient_count; i++)
			sscanf(p.gradient_colors[i], "#%x", &gradCol[i]);
	}
	return 0;
}

void clear_screen_sdl() {
	SDL_FillRect(xavaSDLWindowSurface, NULL, SDL_MapRGB(xavaSDLWindowSurface->format, p.bgcol/0x10000%0x100, p.bgcol/0x100%0x100, p.bgcol%0x100));
}

void apply_window_settings_sdl() {
	// toggle fullscreen
	SDL_SetWindowFullscreen(xavaSDLWindow, SDL_WINDOW_FULLSCREEN & fs);

	xavaSDLWindowSurface = SDL_GetWindowSurface(xavaSDLWindow);
	// Appearently SDL uses multithreading so this avoids invalid access
	// If I had a job, here's what I would be fired for xD
	SDL_Delay(100);
	clear_screen_sdl();
	
	// Window size patch, because xava wipes w and h for some reason.
	p.w = xavaSDLWindowSurface->w;
	p.h = xavaSDLWindowSurface->h;
	return;
}

int get_window_input_sdl() {
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
						fs = !fs;
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
						if(p.gradient) break;
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
 
void draw_graphical_sdl(int bars, int rest, int *f, int *flastd) {
	for(int i = 0; i < bars; i++) {
		SDL_Rect current_bar;
		if(f[i] > flastd[i]){ 
			if(!p.gradient) {
				current_bar = (SDL_Rect) {rest + i*(p.bs+p.bw), p.h - f[i], p.bw, f[i] - flastd[i]};
				SDL_FillRect(xavaSDLWindowSurface, &current_bar, SDL_MapRGB(xavaSDLWindowSurface->format, p.col/0x10000%0x100, p.col/0x100%0x100, p.col%0x100));
			} else {
				for(unsigned int I = (unsigned int)flastd[i]; I < (unsigned int)f[i]; I++) {
					Uint32 color = 0xFF;
					double step = (double)(I%((unsigned int)p.h/(p.gradient_count-1)))/(double)((double)p.h/(p.gradient_count-1));
					color = color << 8;
					
					unsigned int gcPhase = (p.gradient_count-1)*I/(unsigned int)p.h;
					if(gradCol[gcPhase] / 0x10000 % 0x100 < gradCol[gcPhase+1] / 0x10000 % 0x100)
						color += step*(gradCol[gcPhase+1] / 0x10000 % 0x100 - gradCol[gcPhase] / 0x10000 % 0x100) + gradCol[gcPhase] / 0x10000 % 0x100;
					else
						color += -1*step*(gradCol[gcPhase] / 0x10000 % 0x100 - gradCol[gcPhase+1] / 0x10000 % 0x100) + gradCol[gcPhase] / 0x10000 % 0x100;
					color = color << 8;
					if(gradCol[gcPhase] / 0x100 % 0x100 < gradCol[gcPhase+1] / 0x100 % 0x100)
						color += step*(gradCol[gcPhase+1] / 0x100 % 0x100 - gradCol[gcPhase] / 0x100 % 0x100) + gradCol[gcPhase] / 0x100 % 0x100;
					else
						color += -1*step*(gradCol[gcPhase] / 0x100 % 0x100 - gradCol[gcPhase+1] / 0x100 % 0x100) + gradCol[gcPhase] / 0x100 % 0x100;
					color = color << 8;
					if(gradCol[gcPhase] % 0x100 < gradCol[gcPhase+1] % 0x100)
						color += step*(gradCol[gcPhase+1] % 0x100 - gradCol[gcPhase] % 0x100) + gradCol[gcPhase] % 0x100;
					else
						color += -1*step*(gradCol[gcPhase] % 0x100 - gradCol[gcPhase+1] % 0x100) + gradCol[gcPhase] % 0x100;
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
