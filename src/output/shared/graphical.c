#include <string.h>

#include "graphical.h"

#define SET_INNER_X(val) \
    xava->inner.x = ((xava->outer.w == xava->inner.w)? 0 : (val))
#define SET_INNER_Y(val) \
    xava->inner.y = ((xava->outer.h == xava->inner.h)? 0 : (val))

void __internal_xava_graphical_calculate_win_pos_keep(struct XAVA_HANDLE *xava,
                                            uint32_t winW, uint32_t winH) {
    struct config_params *conf = &xava->conf;

    xava->outer.w = winW;
    xava->outer.h = winH;
    xava->inner.w = conf->w;
    xava->inner.h = conf->h;
    // skip resetting the outer x, as those can be troublesome
    xava->inner.x = 0;
    xava->inner.y = 0;

    if(xava->outer.w <= conf->w)
        xava->inner.w = xava->outer.w;
    if(xava->outer.h <= conf->h)
        xava->inner.h = xava->outer.h;

    if(!strcmp(conf->winA, "top")) {
        SET_INNER_X((winW - conf->w) / 2 + conf->x);
        SET_INNER_Y(0                    + conf->y);
    } else if(!strcmp(conf->winA, "bottom")) {
        SET_INNER_X((winW - conf->w) / 2 + conf->x);
        SET_INNER_Y(winH - conf->h       - conf->y);
    } else if(!strcmp(conf->winA, "top_left")) {
        SET_INNER_X(0                    + conf->x);
        SET_INNER_Y(0                    + conf->y);
    } else if(!strcmp(conf->winA, "top_right")) {
        SET_INNER_X(winW - conf->w       - conf->x);
        SET_INNER_Y(0                    + conf->y);
    } else if(!strcmp(conf->winA, "left")) {
        SET_INNER_X(0                    + conf->x);
        SET_INNER_Y((winH - conf->h) / 2 + conf->y);
    } else if(!strcmp(conf->winA, "right")) {
        SET_INNER_X(winW - conf->w       - conf->x);
        SET_INNER_Y((winH - conf->h) / 2 + conf->y);
    } else if(!strcmp(conf->winA, "bottom_left")) {
        SET_INNER_X(0                    + conf->x);
        SET_INNER_Y(winH - conf->h       - conf->y);
    } else if(!strcmp(conf->winA, "bottom_right")) {
        SET_INNER_X(winW - conf->w       - conf->x);
        SET_INNER_Y(winH - conf->h       - conf->y);
    } else if(!strcmp(conf->winA, "center")) {
        SET_INNER_X((winW - conf->w) / 2 + conf->x);
        SET_INNER_Y((winH - conf->h) / 2 + conf->y);
    }
}

void __internal_xava_graphical_calculate_win_pos_nokeep(struct XAVA_HANDLE *xava,
                                        uint32_t scrW, uint32_t scrH,
                                        uint32_t winW, uint32_t winH) {
    struct config_params *conf = &xava->conf;

    xava->outer.w = winW;
    xava->outer.h = winH;
    xava->inner.w = winW;
    xava->inner.h = winH;
    xava->inner.x = 0;
    xava->inner.y = 0;
    xava->outer.x = 0;
    xava->outer.y = 0;

    if(!strcmp(conf->winA, "top")) {
        xava->outer.x = (int32_t)(scrW - winW) / 2 + conf->x;
    } else if(!strcmp(conf->winA, "bottom")) {
        xava->outer.x = (int32_t)(scrW - winW) / 2 + conf->x;
        xava->outer.y = (int32_t)(scrH - winH)     - conf->y;
    } else if(!strcmp(conf->winA, "top_left")) {
        // noop
    } else if(!strcmp(conf->winA, "top_right")) {
        xava->outer.x = (int32_t)(scrW - winW)     - conf->x;
    } else if(!strcmp(conf->winA, "left")) {
        xava->outer.y = (int32_t)(scrH - winH) / 2;
    } else if(!strcmp(conf->winA, "right")) {
        xava->outer.x = (int32_t)(scrW - winW)     - conf->x;
        xava->outer.y = (int32_t)(scrH - winH) / 2 + conf->y;
    } else if(!strcmp(conf->winA, "bottom_left")) {
        xava->outer.y = (int32_t)(scrH - winH)     - conf->y;
    } else if(!strcmp(conf->winA, "bottom_right")) {
        xava->outer.x = (int32_t)(scrW - winW)     - conf->x;
        xava->outer.y = (int32_t)(scrH - winH)     - conf->y;
    } else if(!strcmp(conf->winA, "center")) {
        xava->outer.x = (int32_t)(scrW - winW) / 2 + conf->x;
        xava->outer.y = (int32_t)(scrH - winH) / 2 + conf->y;
    }
}

void calculate_win_geo(struct XAVA_HANDLE *xava, uint32_t winW, uint32_t winH) {
    if(xava->conf.holdSizeF) {
        __internal_xava_graphical_calculate_win_pos_keep(xava, winW, winH);
    } else {
        xava->outer.w = winW;
        xava->outer.h = winH;
        xava->inner.w = winW;
        xava->inner.h = winH;
        // skip resetting the outer x, as those can be troublesome
        xava->inner.x = 0;
        xava->inner.y = 0;
    }
}

void calculate_win_pos(struct XAVA_HANDLE *xava, uint32_t scrW, uint32_t scrH,
                        uint32_t winW, uint32_t winH) {
    if(xava->conf.holdSizeF) {
        __internal_xava_graphical_calculate_win_pos_keep(xava, scrW, scrH);
    } else {
        __internal_xava_graphical_calculate_win_pos_nokeep(xava, scrW, scrH,
                                                                winW, winH);
    }

    // Some error checking
    xavaLogCondition(xava->outer.x > (int)(scrW-winW),
            "Screen out of bounds (X axis) (%d %d %d %d)",
            scrW, scrH, xava->outer.x, winW);
    xavaLogCondition(xava->outer.y > (int)(scrH-winH),
            "Screen out of bounds (Y axis) (%d %d %d %d)",
            scrW, scrH, xava->outer.y, winH);
}

