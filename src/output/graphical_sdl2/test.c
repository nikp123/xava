#include <stdbool.h>
#include <SDL2/SDL.h>

bool am_i_sdl2(void) {
    int val = SDL_Init(SDL_INIT_EVERYTHING);
    if(val != 0)
        return false;

    SDL_Quit();
    return true;
}
