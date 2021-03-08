#include "render.h"

uint32_t extern inline wayland_color_blend(uint32_t color, uint16_t alpha) {
	uint8_t red =   ((color >> 16 & 0xff) | (color >> 8 & 0xff00)) * alpha / 0xffff;
	uint8_t green = ((color >>  8 & 0xff) | (color >> 0 & 0xff00)) * alpha / 0xffff;
	uint8_t blue =  ((color >>  0 & 0xff) | (color << 8 & 0xff00)) * alpha / 0xffff;
	return alpha<<24|red<<16|green<<8|blue;
}

