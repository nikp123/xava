#ifndef __XAVA_OUTPUT_SHARED_CAIRO_FEATURE_COMPAT_H
#define __XAVA_OUTPUT_SHARED_CAIRO_FEATURE_COMPAT_H
// Since drawing full frames is expensive, this "feature checker" is put in
// place of it

#include <stdbool.h>

// this is a bitmask so you'll have to OR these values together
typedef enum XAVA_CAIRO_FEATURE {
    // reports that the cairo module can only draw full frames, trashing any
    // old data in the process. all other modules must support this for it to
    // work
    XAVA_CAIRO_FEATURE_FULL_DRAW = 1,
    // reports that the cairo module can only in certain regions and that it's
    // drawing operation depends on the old data and therefore is unsafe if
    // those regions conflict
    XAVA_CAIRO_FEATURE_DRAW_REGION = 2,
    // reports that the cairo module draws in a certain regions, but in a safe
    // manner meaning that it can "overwrite" previous data without depending
    // on the state of the fremebuffer. or in other words, "safe"
    XAVA_CAIRO_FEATURE_DRAW_REGION_SAFE = 4,
} XAVA_CAIRO_FEATURE;

bool xava_cairo_feature_compatibility_assert(XAVA_CAIRO_FEATURE a);

#endif //__XAVA_OUTPUT_SHARED_CAIRO_FEATURE_COMPAT_H

