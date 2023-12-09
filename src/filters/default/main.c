#include <assert.h>
#include <stdbool.h>
#include <math.h>

#include <fftw3.h>

#include "shared.h"

#ifndef max
    #define max(a,b) \
         ({ __typeof__ (a) _a = (a); \
         __typeof__ (b) _b = (b); \
         _a > _b ? _a : _b; })
#endif

// scalers for reversing chages
#define MONSTERCAT_SCALE  1.5
#define GRAVITY_SCALE    (50.0/100.0)
#define INTEGRAL_SCALE   0.01

typedef struct XAVA_FILTER_CONFIG {
    const uint32_t lowcf, highcf; // low and high cutoff frequency
    const uint32_t overshoot;
    const uint32_t waves;

    const float integral;
    const float ignore;
    const float logScale;
    const float monstercat;
    const float gravity;
    const float eqBalance;

    XAVA_CONFIG_OPTION(bool, oddoneout);
} XAVA_FILTER_CONFIG;

typedef struct XAVA_FILTER_DATA {
    // i wont even bother deciphering this mess
    float    *fpeak, *k, g;

    uint32_t *f, *flastd;  // current and last bars
                                  //
    uint32_t *lcf, *hcf, *fl, *fr, *fmem;
    uint64_t *fall;
    int32_t  *flast; // if you change this to uint32_t you die >:(
    float    *smooth;
    float    smh;
    uint32_t smcount, calcbars;

    fftwf_plan pl, pr;
    fftwf_complex *outl, *outr;

    bool senseLow;
    const XAVA_FILTER_CONFIG config;
    XAVA_FILTER_CONFIG state;
} XAVA_FILTER_DATA;

void separate_freq_bands(fftwf_complex *out,
        uint32_t channel,
        double sens,
        uint32_t fftsize,
        XAVA_FILTER_DATA *data) {
    uint32_t o, i;
    double y[fftsize / 2 + 1];
    double temp;

    // process: separate frequency bands
    for (o = 0; o < data->calcbars; o++) {
        float peak = 0;

        // process: get peaks
        for (i = data->lcf[o]; i <= data->hcf[o]; i++) {
            //getting r of compex
            y[i] = hypot(out[i][0], out[i][1]);
            peak += y[i]; //adding upp band
        }

        peak = peak / (data->hcf[o]-data->lcf[o] + 1); //getting average
        temp = peak * sens * data->k[o] / 800000; //multiplying with k and sens
        if (temp <= data->state.ignore) temp = 0;
        if (channel == 1) data->fl[o] = temp;
        else data->fr[o] = temp;
    }
}


void monstercat_filter(int bars,
        XAVA_FILTER_DATA *data,
        uint32_t *f) {
    int z;

    // process [smoothing]: monstercat-style "average"
    int m_y, de;
    if (data->state.waves > 0) {
        for (z = 0; z < bars; z++) { // waves
            f[z] = f[z] / 1.25;
            //if (f[z] < 1) f[z] = 1;
            for (m_y = z - 1; m_y >= 0; m_y--) {
                de = z - m_y;
                f[m_y] = max(f[z] - pow(de, 2), f[m_y]);
            }
            for (m_y = z + 1; m_y < bars; m_y++) {
                de = m_y - z;
                f[m_y] = max(f[z] - pow(de, 2), f[m_y]);
            }
        }
    } else if (data->state.monstercat > 0) {
        for (z = 0; z < bars; z++) {
            //if (f[z] < 1)f[z] = 1;
            for (m_y = z - 1; m_y >= 0; m_y--) {
                de = z - m_y;
                f[m_y] = max(f[z] / pow(data->state.monstercat, de), f[m_y]);
            }
            for (m_y = z + 1; m_y < bars; m_y++) {
                de = m_y - z;
                f[m_y] = max(f[z] / pow(data->state.monstercat, de), f[m_y]);
            }
        }
    }
}

EXP_FUNC int xavaFilterInit(XAVA *xava) {
    XAVA_AUDIO         *audio = &xava->audio;
    XAVA_CONFIG            *p = &xava->conf;
    XAVA_FILTER_DATA    *data = xava->filter.data;

    // fft: planning to rock
    CALLOC_SELF(data->outl, audio->fftsize/2+1);
    data->pl = fftwf_plan_dft_r2c_1d(audio->fftsize, audio->audio_out_l, data->outl, FFTW_MEASURE);

    if(p->stereo) {
        CALLOC_SELF(data->outr, audio->fftsize/2+1);
        data->pr = fftwf_plan_dft_r2c_1d(audio->fftsize, audio->audio_out_r, data->outr, FFTW_MEASURE);
    }

    xavaBailCondition(data->state.highcf > audio->rate / 2,
            "Higher cutoff cannot be higher than the sample rate / 2");

    arr_init_n(data->fpeak,  xava->bars);
    arr_init_n(data->k,      xava->bars);
    arr_init_n(data->f,      xava->bars);
    arr_init_n(data->lcf,    xava->bars);
    arr_init_n(data->hcf,    xava->bars);
    arr_init_n(data->fmem,   xava->bars);
    arr_init_n(data->flast,  xava->bars);
    arr_init_n(data->flastd, xava->bars);
    arr_init_n(data->fall,   xava->bars);
    arr_init_n(data->fl,     xava->bars);
    arr_init_n(data->fr,     xava->bars);

    return 0;
}

EXP_FUNC void xavaFilterApply(XAVA *xava) {
    XAVA_AUDIO *audio = &xava->audio;
    XAVA_CONFIG *p  = &xava->conf;
    XAVA_FILTER_DATA *data = xava->filter.data;

    // if fpeak is not cleared that means that the variables are not initialized
    if(xava->bars == 0)
        xava->bars = 1;
    arr_resize(data->fpeak,  xava->bars);
    arr_resize(data->k,      xava->bars);
    arr_resize(data->f,      xava->bars);
    arr_resize(data->lcf,    xava->bars);
    arr_resize(data->hcf,    xava->bars);
    arr_resize(data->fmem,   xava->bars);
    arr_resize(data->flast,  xava->bars);
    arr_resize(data->flastd, xava->bars);
    arr_resize(data->fall,   xava->bars);
    arr_resize(data->fl,     xava->bars);
    arr_resize(data->fr,     xava->bars);

    // oddoneout only works if the number of bars is odd, go figure
    if(data->state.oddoneout) {
        if (!(xava->bars%2))
            xava->bars--;
    }

    // update pointers for the main handle
    xava->f  = data->f;
    xava->fl = data->flastd;

    u64 time = xavaGetTime();
    for (uint32_t i = 0; i < xava->bars; i++) {
        data->flast[i] = 0;
        data->flastd[i] = 0;
        data->fall[i] = time;
        data->fpeak[i] = 0;
        data->fmem[i] = 0;
        data->f[i] = 0;
        data->lcf[i] = 0;
        data->hcf[i] = 0;
        data->fl[i] = 0;
        data->fr[i] = 0;
        data->k[i] = .0;
    }

    // process [smoothing]: calculate gravity
    data->g = data->state.gravity * ((float)(xava->inner.h-1) / 2160);

    // checks if there is stil extra room, will use this to center
    xava->rest = MIN(0, (int32_t)(xava->inner.w - xava->bars * p->bw - xava->bars * p->bs + p->bs) / 2);

    if (p->stereo) xava->bars = xava->bars / 2; // in stereo onle half number of bars per channel

    if ((data->smcount > 0) && (xava->bars > 0)) {
        data->smh = (double)(((double)data->smcount)/((double)xava->bars));
    }

    // since oddoneout requires every odd bar, why not split them in half?
    data->calcbars = data->state.oddoneout ? xava->bars/2+1 : xava->bars;

    // frequency constant that we'll use for logarithmic progression of frequencies
    double freqconst = log(data->state.highcf-data->state.lowcf) /
      log(pow(data->calcbars, data->state.logScale));
    //freqconst = -2;

    // process: calculate cutoff frequencies
    uint32_t n;
    for (n=0; n < data->calcbars; n++) {
        float fc = pow(
                powf(n, (data->state.logScale-1.0) *
                    ((double)n+1.0) /
                    ((double)data->calcbars)+1.0),
                freqconst) +
            data->state.lowcf;
        float fre = fc / (audio->rate / 2.0);
        // Remember nyquist!, pr my calculations this should be rate/2
        // and  nyquist freq in M/2 but testing shows it is not...
        // or maybe the nq freq is in M/4

        //lfc stores the lower cut frequency foo each bar in the fft out buffer
        data->lcf[n] = floor(fre * (audio->fftsize/2.0));

        if (n != 0) {
            //hfc holds the high cut frequency for each bar

            // I know it's not precise, but neither are integers
            // You can see why in https://github.com/nikp123/xava/issues/29
            // I did reverse the "next_bar_lcf-1" change
            data->hcf[n-1] = data->lcf[n];
        }

        // process: weigh signal to frequencies height and EQ
        data->k[n] = pow(fc, data->state.eqBalance);
        data->k[n] *= (float)(xava->inner.h-1) / 100;
        data->k[n] *= data->smooth[(int)floor(((double)n) * data->smh)];
    }
    if(data->calcbars > 1)
        data->hcf[n-1] = data->state.highcf*audio->fftsize/audio->rate;

    if (p->stereo)
        xava->bars = xava->bars * 2;
}

EXP_FUNC void xavaFilterLoop(XAVA *xava) {
    XAVA_AUDIO *audio = &xava->audio;
    XAVA_CONFIG *p  = &xava->conf;
    XAVA_FILTER_DATA *data = xava->filter.data;

    // process: execute FFT and sort frequency bands
    if (p->stereo) {
        fftwf_execute(data->pl);
        fftwf_execute(data->pr);

        separate_freq_bands(data->outl, 1, p->sens, audio->fftsize, data);
        separate_freq_bands(data->outr, 2, p->sens, audio->fftsize, data);
    } else {
        fftwf_execute(data->pl);
        separate_freq_bands(data->outl, 1, p->sens, audio->fftsize, data);
    }

    if(data->state.oddoneout) {
        for(int i=xava->bars/2; i>0; i--) {
            data->fl[i*2-1+xava->bars%2]=data->fl[i];
            data->fl[i]=0;
        }
    }

    // process [smoothing]
    if (data->state.monstercat) {
        if (p->stereo) {
            monstercat_filter(xava->bars / 2, data, data->fl);
            monstercat_filter(xava->bars / 2, data, data->fr);
        } else {
            monstercat_filter(data->calcbars, data, data->fl);
        }
    }

    //preperaing signal for drawing
    for (uint32_t i=0; i<xava->bars; i++) {
        if (p->stereo) {
            if (i < xava->bars / 2) {
                data->f[i] = data->fl[xava->bars / 2 - i - 1];
            } else {
                data->f[i] = data->fr[i - xava->bars / 2];
            }
        } else {
            data->f[i] = data->fl[i];
        }
    }


    // process [smoothing]: falloff
    if (data->g > 0) {
        for (uint32_t i = 0; i < xava->bars; i++) {
            if ((int32_t)data->f[i] < data->flast[i]) {
                /**
                 * Big explaination here, because if you touch this IT'LL break
                 *
                 * Basically the trick is that since this equation can yield a
                 * negative value it also world overflow the f[] and thats NO
                 * good!
                 *
                 * But the negative value is useful indeed because it smooths
                 * the integral output and hence provides a better visualization
                 * experience.
                 **/
                float time_diff = (xavaGetTime() - data->fall[i]) / 16.0f;

                data->flast[i] = data->fpeak[i] -
                    (data->g * time_diff * time_diff);
                data->f[i] = MIN(0, data->flast[i]);
                data->fall[i]++;
            } else  {
                data->fpeak[i] = data->f[i];
                data->fall[i] = 0;
            }
        }
    }

    // process [smoothing]: integral
    if (data->state.integral > 0) {
        for (uint32_t i=0; i<xava->bars; i++) {
            data->f[i] = data->fmem[i] * data->state.integral + data->f[i];
            data->fmem[i] = data->f[i];

            int diff = xava->inner.h - data->f[i];
            if (diff < 0) diff = 0;
            double div = 1.0 / (double)(diff + 1);
            //f[o] = f[o] - pow(div, 10) * (height + 1);
            data->fmem[i] = data->fmem[i] * (1 - div / 20);
        }
    }

    // process [oddoneout]
    if(data->state.oddoneout) {
        for(int32_t i=1; i<(int32_t)xava->bars-1; i+=2) {
            data->f[i] = (data->f[i+1] + data->f[i-1])/2;
        }
        for(int32_t i=(int32_t)xava->bars-3; i>1; i-=2) {
            uint32_t sum = (data->f[i+1] + data->f[i-1])/2;
            if(sum>data->f[i]) data->f[i] = sum;
        }
    }

    data->senseLow = true;

    // automatic sens adjustment
    if (p->autosens&&(!xava->pauseRendering)) {
        // don't adjust on complete silence
        // as when switching tracks for example
        for (uint32_t i=0; i<xava->bars; i++) {
            if (data->f[i] > ((xava->inner.h-1)*(100+data->state.overshoot)/100) ) {
                data->senseLow = false;
                p->sens *= 0.985;
                break;
            }
        }
        if (data->senseLow) {
            p->sens *= 1.001;
        }
    }
}

EXP_FUNC void xavaFilterCleanup(XAVA *xava) {
    XAVA_FILTER_DATA *data = xava->filter.data;
    XAVA_CONFIG *p = &xava->conf;

    free(data->outl);
    fftwf_destroy_plan(data->pl);
    if(p->stereo) {
        free(data->outr);
        fftwf_destroy_plan(data->pr);
    }
    fftwf_cleanup();

    free(data->smooth);

    // honestly even I don't know what these mean
    // but for the meantime, they are moved here and I won't touch 'em
    arr_free(data->fpeak);
    arr_free(data->k);
    arr_free(data->f);
    arr_free(data->lcf);
    arr_free(data->hcf);
    arr_free(data->fmem);
    arr_free(data->flast);
    arr_free(data->flastd);
    arr_free(data->fall);
    arr_free(data->fl);
    arr_free(data->fr);

    free(data);
}

XAVA_FILTER_CONFIG generate_default_config(xava_config_source config) {
    XAVA_CONFIG_OPTION(bool, oddoneout);

    XAVA_CONFIG_GET_BOOL(config, "filter", "oddoneout", true, oddoneout);
    return (XAVA_FILTER_CONFIG) {
        .lowcf     = xavaConfigGetI32(config, "filter", "lower_cutoff_freq",  26),
        .highcf    = xavaConfigGetI32(config, "filter", "higher_cutoff_freq", 15000),
        .overshoot = xavaConfigGetI32(config, "filter", "overshoot", 0),

        .waves     = xavaConfigGetI32(config, "filter", "waves", 0),

        .integral   = xavaConfigGetF64(config, "filter", "integral", 85)
            * INTEGRAL_SCALE,

        .ignore     = xavaConfigGetF64(config, "filter", "ignore", 0),
        .logScale   = xavaConfigGetF64(config, "filter", "log", 1.55),

        .monstercat = xavaConfigGetF64(config, "filter", "monstercat", 0.0)
            * MONSTERCAT_SCALE,
        .gravity    = xavaConfigGetF64(config, "filter", "gravity", 100)
            * GRAVITY_SCALE,

        .oddoneout = oddoneout,
        .oddoneout_is_set_from_file = oddoneout_is_set_from_file,

        .eqBalance  = xavaConfigGetF64(config, "filter", "eq_balance", 0.67),
    };
}

EXP_FUNC void xavaFilterLoadConfig(XAVA *xava) {
    XAVA_CONFIG *p = &xava->conf;
    XAVA_AUDIO  *a = &xava->audio;

    xava_config_source config = xava->default_config.config;

    // scope made for safety reasons
    {
        // alloc the thing
        XAVA_FILTER_DATA data = {
            .config = generate_default_config(config),
            .state  = generate_default_config(config),
        };

        xava->filter.data = malloc(sizeof(XAVA_FILTER_DATA));
        memcpy(xava->filter.data, &data, sizeof(XAVA_FILTER_DATA));
    }

    XAVA_FILTER_DATA *data = xava->filter.data;
    XAVA_FILTER_CONFIG *state = &data->state;

    XAVA_CONFIG_GET_F64(config, "filter", "sensitivity", 100.0, p->sens);
    p->sens *= XAVA_PREDEFINED_SENS_VALUE; // check shared.h for details
    XAVA_CONFIG_GET_BOOL(config, "filter", "autosens", true, p->autosens);


    // read & validate: eq
    data->smcount = xavaConfigGetKeyNumber(config, "eq");
    if (data->smcount > 0) {
        MALLOC_SELF(data->smooth, data->smcount);
        char **keys = xavaConfigGetKeys(config, "eq");
        for (uint32_t sk = 0; sk < data->smcount; sk++) {
            data->smooth[sk] = xavaConfigGetF64(config, "eq", keys[sk], 1);
        }
        free(keys);
    } else {
        data->smcount = 64; //back to the default one
        MALLOC_SELF(data->smooth, data->smcount);
        for(int i=0; i<64; i++) data->smooth[i]=1.0f;
    }

    // validate: gravity
    xavaBailCondition(state->gravity < 0, "Gravity cannot be below 0");

    // validate: oddoneout
    if(data->config.oddoneout && p->stereo) { // incompatible hence must be processed
        xavaBailCondition(data->config.oddoneout_is_set_from_file && p->stereo_is_set_from_file,
            "Cannot have oddoneout and stereo enabled AT THE SAME TIME!");

        // fix config in both cases
        if(data->config.oddoneout_is_set_from_file) {
            p->stereo = false;
            a->channels = 1;
        } else if(p->stereo_is_set_from_file) {
            state->oddoneout = false;
        } else {
            xavaBail("[BUG] Tell nik to fix those stupid defaults!");
        }
    }

    // validate: integral
    xavaBailCondition(state->integral < 0,
        "Integral cannot be below 0");
    xavaBailCondition(state->integral > 1,
        "Integral cannot be above 100");

    // validate: cutoff
    xavaBailCondition(state->lowcf <= 0,
        "Lower cutoff frequency CANNOT be 0 or below");
    xavaBailCondition(state->lowcf > state->highcf,
            "Lower frequency cutoff cannot be higher than the higher cutoff\n");
}
