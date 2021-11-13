#ifndef H_GRAPHICAL
#define H_GRAPHICAL

#include "../shared.h"

void calculate_win_pos(struct config_params *p, uint32_t scrW, uint32_t scrH);
void calculate_inner_win_pos(struct XAVA_HANDLE *xava, unsigned int newW,
        unsigned int newH);

#define DEF_FG_COL 6
#define DEF_BG_COL 0
#define COLOR_NUM 8
static const unsigned int colorNumbers[] = {0x000000, 0xFF0000, 0x00FF00, 0xFFFF00,
                                            0x0000FF, 0xFF00FF, 0x00FFFF, 0xFFFFFF};

#define ARGB_A_32(x) ((x>>24)&0xff)
#define ARGB_R_32(x) ((x>>16)&0xff)
#define ARGB_G_32(x) ((x>>8)&0xff)
#define ARGB_B_32(x) (x&0xff)

#define A_ARGB_32(x) ((x&0xff)<<24)
#define R_ARGB_32(x) ((x&0xff)<<16)
#define G_ARGB_32(x) ((x&0xff)<<8)
#define B_ARGB_32(x) (x&0xff)

#define ARGB_A_64(x) ((x>>48)&0xffff)
#define ARGB_R_64(x) ((x>>32)&0xffff)
#define ARGB_G_64(x) ((x>>16)&0xffff)
#define ARGB_B_64(x) (x&0xffff)

#define A_ARGB_64(x) ((x&0xffff)<<49)
#define R_ARGB_64(x) ((x&0xffff)<<32)
#define G_ARGB_64(x) ((x&0xffff)<<16)
#define B_ARGB_64(x) (x&0xffff)

#define TRANS_FROM_MIN(x, y, z) ((typeof(x))(DIFF(x,y)*z)+MIN(x,y))
#define TRANS_FROM_MAX(x, y, z) (MAX(x,y)-(typeof(x))(DIFF(x,y)*z))

#define UNSIGNED_TRANS(x, y, z) (x>y? TRANS_FROM_MAX(x,y,z) : TRANS_FROM_MIN(x,y,z))

#endif
