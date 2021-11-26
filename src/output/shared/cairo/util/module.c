#include "module.h"
#include "region.h"
#include "feature_compat.h"
#include "array.h"

XAVA_CAIRO_FEATURE xava_cairo_module_check_compatibility(xava_cairo_module *modules) {
    XAVA_CAIRO_FEATURE lowest_common_denominator =
        XAVA_CAIRO_FEATURE_DRAW_REGION_SAFE |
        XAVA_CAIRO_FEATURE_DRAW_REGION |
        XAVA_CAIRO_FEATURE_FULL_DRAW;

    // check whether there are feature misalignments with modules
    for(size_t i = 0; i < arr_count(modules); i++) {
        xavaReturnErrorCondition(!xava_cairo_feature_compatibility_assert(modules[i].features),
                0, "Module sanity check failed for \'%s\'", modules[i].name);
        lowest_common_denominator &= modules[i].features;
    }
    xavaReturnErrorCondition(lowest_common_denominator == 0,
            0, "Modules do not have inter-compatible drawing methods!\n",
            "Please report this to the developer WHILST including your "
            "current configuration.");

    // probably will never happen but who knows ¯\_ (ツ)_/¯
    xavaReturnSpamCondition(lowest_common_denominator & XAVA_CAIRO_FEATURE_DRAW_REGION_SAFE,
            XAVA_CAIRO_FEATURE_DRAW_REGION_SAFE,
            "Skipping region bounds checking as all of them are safe!");

    // if regions arent supported, full draws MUST be
    if((lowest_common_denominator & XAVA_CAIRO_FEATURE_DRAW_REGION) == 0) {
        xavaReturnSpamCondition(lowest_common_denominator & XAVA_CAIRO_FEATURE_FULL_DRAW,
                XAVA_CAIRO_FEATURE_FULL_DRAW,
                "Skipping region bounds checking as all of them are drawing the frame anyway!");

        xavaBail("No common drawing mode exists. Most likely a BUG!");
    }

    bool pass = true;
    for(size_t i = 0; i < arr_count(modules)-1; i++) {
        for(size_t j = i+1; j < arr_count(modules); j++) {
            if(xava_cairo_region_list_check(modules[i].regions,
                        modules[j].regions)) {
                pass = false;
            }
        }
    }

    if(pass == false) {
        xavaBailCondition(
                (lowest_common_denominator & XAVA_CAIRO_FEATURE_FULL_DRAW) == 0,
                "No common drawing mode exists since no regions match. RIP.");
        return XAVA_CAIRO_FEATURE_FULL_DRAW;
    }

    return XAVA_CAIRO_FEATURE_DRAW_REGION;
}
