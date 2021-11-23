#include "array.h"
#include "region.h"

// returns false on pass
// returns true on region conflict
bool xava_cairo_region_list_check(xava_cairo_region *A,
        xava_cairo_region *B) {
    // computational complexity high == low bugs (hopefully)
    for(size_t a = 0; a < arr_count(A); a++) {
        for(size_t b = 0; b < arr_count(B); b++) {
            if(A[a].x+A[a].w < B[b].x) continue; // outside of bounds X positive
            if(A[a].x > B[b].x+B[b].w) continue; // outside of bounds X negative
            if(A[a].y+A[a].h < B[b].y) continue; // outside of bounds Y positive
            if(A[a].y > B[b].y+B[b].h) continue; // outside of bounds Y negative
            return true; // all checks failing meaning a box intersection occured
        }
    }
    return false;
}

