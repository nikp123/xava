#include <SDL2/SDL.h>
#include <time.h>
#include "graphical.h"

SDL_Window *xavaSDLWindow;
SDL_Surface *xavaSDLWindowSurface;
SDL_Event xavaSDLEvent;
SDL_DisplayMode xavaSDLVInfo;
int *gradCol, gradNum;

void cleanup_graphical_sdl(void)
{
	free(gradCol);
	SDL_FreeSurface(xavaSDLWindowSurface);
	SDL_DestroyWindow(xavaSDLWindow);
	SDL_Quit();
}

int init_window_sdl(int *col, int *bgcol, char *color, char *bcolor, int gradient, char **gradient_colors, int gradient_num, int w, int h)
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
		windowX = (xavaSDLVInfo.w - w) / 2 + windowX;
	}else if(!strcmp(windowAlignment, "bottom")){
		windowX = (xavaSDLVInfo.w - w) / 2 + windowX;
		windowY = (xavaSDLVInfo.h - h) + (-1*windowY);
	}else if(!strcmp(windowAlignment, "top_left")){
		// Nothing to do here :P
	}else if(!strcmp(windowAlignment, "top_right")){
		windowX = (xavaSDLVInfo.w - w) + (-1*windowX);
	}else if(!strcmp(windowAlignment, "left")){
		windowY = (xavaSDLVInfo.h - h) / 2;
	}else if(!strcmp(windowAlignment, "right")){
		windowX = (xavaSDLVInfo.w - w) + (-1*windowX);
		windowY = (xavaSDLVInfo.h - h) / 2 + windowY;
	}else if(!strcmp(windowAlignment, "bottom_left")){
		windowY = (xavaSDLVInfo.h - h) + (-1*windowY);
	}else if(!strcmp(windowAlignment, "bottom_right")){
		windowX = (xavaSDLVInfo.w - w) + (-1*windowX);
		windowY = (xavaSDLVInfo.h - h) + (-1*windowY);
	}else if(!strcmp(windowAlignment, "center")){
		windowX = (xavaSDLVInfo.w - w) / 2 + windowX;
		windowY = (xavaSDLVInfo.h - h) / 2 + windowY;
	}
	// creating a window
	Uint32 windowFlags = SDL_WINDOW_RESIZABLE;
	if(fs) windowFlags |= SDL_WINDOW_FULLSCREEN;
	if(!borderFlag) windowFlags |= SDL_WINDOW_BORDERLESS;
	xavaSDLWindow = SDL_CreateWindow("XAVA", windowX, windowY, w, h, windowFlags);
	if(!xavaSDLWindow)
	{
		fprintf(stderr, "SDL window cannot be created: %s\n", SDL_GetError());
		SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "ERROR", "cannot create SDL window", NULL);
		SDL_Quit();
		return 1;
	}
	if(color[0] != '#')
	{
		switch((*col))
		{
			case 0:
				(*col) = 0x000000;
				break;
			case 1:
				(*col) = 0xFF0000;
				break;
			case 2:
				(*col) = 0x00FF00;
				break;
			case 3:
				(*col) = 0xFFFF00;
				break;
			case 4:
				(*col) = 0x0000FF;
				break;
			case 5:
				(*col) = 0xFF00FF;
				break;
			case 6:
				(*col) = 0x00FFFF;
				break;
			case 7:
				(*col) = 0xFFFFFF;
				break;
		}
	}
	else
		sscanf(color+1, "%x", col);
	if(bcolor[0] != '#')
	{
		switch((*bgcol))
		{
			case 0:
				(*bgcol) = 0x000000;
				break;
			case 1:
				(*bgcol) = 0xFF0000;
				break;
			case 2:
				(*bgcol) = 0x00FF00;
				break;
			case 3:
				(*bgcol) = 0xFFFF00;
				break;
			case 4:
				(*bgcol) = 0x0000FF;
				break;
			case 5:
				(*bgcol) = 0xFF00FF;
				break;
			case 6:
				(*bgcol) = 0x00FFFF;
				break;
			case 7:
				(*bgcol) = 0xFFFFFF;
				break;
		}
	}
	else
		sscanf(bcolor, "%x", bgcol);

	if(gradient) {
		gradCol = malloc(sizeof(int)*gradient_num);
		for(int i=0; i<gradient_num; i++)
			sscanf(gradient_colors[i]+1, "%x", &gradCol[i]);
		gradNum = gradient_num;
	}
	return 0;
}

void clear_screen_sdl(int bgcol) {
	SDL_FillRect(xavaSDLWindowSurface, NULL, SDL_MapRGB(xavaSDLWindowSurface->format, bgcol / 0x10000 % 0x100, bgcol / 0x100 % 0x100, bgcol % 0x100));
}

void apply_window_settings_sdl(int bgcol, int *w, int *h)
{
	// toggle fullscreen
	SDL_SetWindowFullscreen(xavaSDLWindow, SDL_WINDOW_FULLSCREEN & fs);

	xavaSDLWindowSurface = SDL_GetWindowSurface(xavaSDLWindow);
	// Appearently SDL uses multithreading so this avoids invalid access
	// If I had a job, here's what I would be fired for xD
	SDL_Delay(100);
	clear_screen_sdl(bgcol);
	
	// Window size patch, because xava wipes w and h for some reason.
	(*w) = xavaSDLWindowSurface->w;
	(*h) = xavaSDLWindowSurface->h;
	return;
}

int get_window_input_sdl(int *bs, int *bw, double *sens, int *col, int *bgcol, int *w, int *h, int gradient)
{
	while(SDL_PollEvent(&xavaSDLEvent) != 0)
	{
		switch(xavaSDLEvent.type)
		{
			case SDL_KEYDOWN:
				switch(xavaSDLEvent.key.keysym.sym)
				{
					// should_reload = 1
					// resizeTerminal = 2
					// bail = -1
					case SDLK_a:
						(*bs)++;
						return 2;
					case SDLK_s:
						if((*bs) > 0) (*bs)--;
						return 2;
					case SDLK_ESCAPE:
						return -1;
					case SDLK_f: // fullscreen
						fs = !fs;
						return 2;
					case SDLK_UP: // key up
						(*sens) *= 1.05;
						break;
					case SDLK_DOWN: // key down
						(*sens) *= 0.95;
						break;
					case SDLK_LEFT: // key left
						(*bw)++;
						return 2;
					case SDLK_RIGHT: // key right
						if((*bw) > 1) (*bw)--;
						return 2;
					case SDLK_r: // reload config
						return 1;
					case SDLK_c: // change foreground color
						if(gradient) break;
						(*col) = rand() % 0x100;
						(*col) = (*col) << 16;
						(*col) += rand();
						return 3;
					case SDLK_b: // change background color
						(*bgcol) = rand() % 0x100;
						(*bgcol) = (*bgcol) << 16;
						(*bgcol) += rand();
						return 3;
					case SDLK_q:
						return -1;
				}
				break;
			case SDL_WINDOWEVENT:
				if(xavaSDLEvent.window.event == SDL_WINDOWEVENT_CLOSE) return -1;
				else if(xavaSDLEvent.window.event == SDL_WINDOWEVENT_RESIZED){
					// if the user resized the window
					(*w) = xavaSDLEvent.window.data1;
					(*h) = xavaSDLEvent.window.data2;
					return 2;
				}
				break;
		}
	}
	return 0;
}
 
void draw_graphical_sdl(int bars, int rest, int bw, int bs, int *f, int *flastd, int col, int bgcol, int gradient, int h) {
	for(int i = 0; i < bars; i++)
	{
		SDL_Rect current_bar;
		if(f[i] > flastd[i]){ 
			if(!gradient)
			{
				current_bar = (SDL_Rect) {rest + i*(bs+bw), h - f[i], bw, f[i] - flastd[i]};
				SDL_FillRect(xavaSDLWindowSurface, &current_bar, SDL_MapRGB(xavaSDLWindowSurface->format, col / 0x10000 % 0x100, col / 0x100 % 0x100, col % 0x100));
			}
			else
			{
				for(int I = flastd[i]; I < f[i]; I++)
				{
					Uint32 color = 0xFF;
					double step = (double)(I%(h/(gradNum-1)))/(double)(h/(gradNum-1));
					color = color << 8;
					
					int gcPhase = (gradNum-1)*I/h;
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
					current_bar = (SDL_Rect) {rest + i*(bs+bw), h - I, bw, 1};
					SDL_FillRect(xavaSDLWindowSurface, &current_bar, SDL_MapRGB(xavaSDLWindowSurface->format, color / 0x10000 % 0x100, color / 0x100 % 0x100, color % 0x100));
				}
			}
		} else if(f[i] < flastd[i]) {
			current_bar = (SDL_Rect) {rest + i*(bs+bw), h - flastd[i], bw, flastd[i] - f[i]};
			SDL_FillRect(xavaSDLWindowSurface, &current_bar, SDL_MapRGB(xavaSDLWindowSurface->format, bgcol / 0x10000 % 0x100, bgcol / 0x100 % 0x100, bgcol % 0x100));
		}
	}
	SDL_UpdateWindowSurface(xavaSDLWindow);
	return;
}
