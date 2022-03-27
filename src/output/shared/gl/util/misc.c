#include <math.h>

#include "misc.h"

float xava_gl_module_util_calculate_intensity(XAVA *xava) {
    XAVA_CONFIG *conf = &xava->conf;

    float intensity = 0.0;

    for(register uint32_t i=0; i<xava->bars; i++) {
        // the not so, speed part
        // intensity has a low-freq bias as they are more "physical"
        float bar_percentage = (float)(xava->f[i]-1)/(float)conf->h;
        if(bar_percentage > 0.0) {
            intensity+=powf(bar_percentage,
                    (float)2.0*(float)i/(float)xava->bars);
        }
    }

    // since im not bothering to do the math, this'll do
    // - used to balance out intensity across various number of bars
    intensity /= xava->bars;

    return intensity;
}

float xava_gl_module_util_obtain_time(void) {
    return (float)fmodl((long double)xavaGetTime()/(long double)1000.0, 3600.0);
}

