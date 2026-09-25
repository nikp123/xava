#include <ctype.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#include "shared.h"
#include "genre.h"

#define GENRE_NAME_MAX_LEN 64

// Grayish color monstercat usually uses
#define UNDEFINED_GENRE_COLOR 0xCCCCCC

struct genre {
    char        name[GENRE_NAME_MAX_LEN];
    uint32_t    color; // represented in 24bit RGB
};

// We're parodying the monstercat color scheme, that's the intention
static const struct genre genre_list[] = {
    { .name = "complextro",       .color = 0xE5E500 }, // Bright Yellow
    { .name = "electro",          .color = 0xE5E500 }, // Bright Yellow
    { .name = "electro house",    .color = 0xE5E500 }, // Bright Yellow
    { .name = "dubstep",          .color = 0x8C00E5 }, // Deep Purple
    { .name = "melodic dubstep",  .color = 0x8C00E5 }, // Deep Purple
    { .name = "drumstep",         .color = 0xE60382 }, // Hot Pink
    { .name = "drum and bass",    .color = 0xE50000 }, // Dark Red
    { .name = "drum & bass",      .color = 0xE50000 }, // Dark Red
    { .name = "dnb",              .color = 0xE50000 }, // Alias for Drum & Bass
    { .name = "house",            .color = 0xE57300 }, // Warm Orange
    { .name = "future house",     .color = 0xE57300 }, // Warm Orange
    { .name = "glitch",           .color = 0x00E53B }, // Neon Green / Mint
    { .name = "glitch hop",       .color = 0x00E53B }, // Neon Green / Mint
    { .name = "moombahcore",      .color = 0x00E53B }, // Often grouped with Glitch Hop
    { .name = "hardcore",         .color = 0x0CB401 }, // Darker green
    { .name = "happy hardcore",   .color = 0x0CB401 }, // Darker green
    { .name = "trap",             .color = 0x81022A }, // Dark red/pink

    // Melodic & Atmospheric Styles
    { .name = "chillstep",        .color = 0x8C00E5 }, // Deep Purple
    { .name = "trance",           .color = 0x004CE5 }, // Royal Blue
    { .name = "psytrance",        .color = 0x004CE5 }, // Royal Blue
    { .name = "future bass",      .color = 0x989FFF }, // Light Pastel Teal
    { .name = "synthwave",        .color = 0x27BBAF }, // Teal
    { .name = "chillout",         .color = 0xCCCCCC }, // Silver
    { .name = "ambient",          .color = 0x00E5E5 }, // Often grouped with Chillout
    { .name = "indie",            .color = 0x27BBAF }, // Teal
    { .name = "indie dance",      .color = 0x27BBAF }, // Teal

    // Traditional & Eclectic Styles
    { .name = "rock",             .color = 0x7F7F7F }, // Industrial Grey
    { .name = "metal",            .color = 0x333333 }, // Charcoal / Off-Black
    { .name = "pop",              .color = 0xFFC0CB }, // Bubblegum Pink
    { .name = "jazz",             .color = 0xD2B48C }, // Smooth Tan / Brass
    { .name = "classical",        .color = 0xFFFDD0 }, // Ivory / Cream White

    // Catch-all Catch Category
    { .name = "electronic",       .color = 0xCCCCCC }, // Silver (Generic Electronic)
    { .name = "edm",              .color = 0xCCCCCC }, // Silver
    { .name = "mix",              .color = 0xFFFFFF }, // Albums are just pure white
};

EXP_FUNC uint32_t xavaGenreToColor(char *genre_string) {
    char string[GENRE_NAME_MAX_LEN] = { 0 };
    bool found = false;
    uint32_t color;

    // copy the genre string
    strncpy(string, genre_string, GENRE_NAME_MAX_LEN-1);

    // lowerspace the genre string so we have a smaller search space
    for(size_t i = 0; i < strlen(string); i++) {
        string[i] = tolower(string[i]);
    }

    // simple look up table thing
    for(long unsigned i = 0; i < sizeof(genre_list)/sizeof(struct genre); i++) {
        if(!strncmp(string, genre_list[i].name, GENRE_NAME_MAX_LEN)) {
            found = true;
            color = genre_list[i].color;
        }
    }

    // safe fallback for when the genre is unknown
    if(found == false)
        color = UNDEFINED_GENRE_COLOR;

    return color;
}
