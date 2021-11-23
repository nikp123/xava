#ifndef __XAVA_OUTPUT_SHARED_CAIRO_REGION_H
#define __XAVA_OUTPUT_SHARED_CAIRO_REGION_H

#include <stdbool.h>
// keeping bounding boxes constrained to well,... rectangles. we can make this
// system way simpler than do to bitmask checks which is stupidly memory
// inefficient and computationally expensive
typedef struct xava_cairo_region {
    int x; // region starting x coordinate
    int y; // region starting y coordinate
    int w; // region starting w coordinate
    int h; // region starting h coordinate
} xava_cairo_region;

// functions
bool xava_cairo_region_list_check(xava_cairo_region *A,
        xava_cairo_region *B);

#endif

