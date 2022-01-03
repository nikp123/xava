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

// i wont even bother deciphering this mess
static float *fc, *fre, *fpeak, *k, g;
static int *f, *lcf, *hcf, *fmem, *flast, *flastd, *fall, *fl, *fr;
static float* smooth, gravity, integral, eqBalance, logScale, ignore;
static float monstercat, *peak, smh;
static bool oddoneout;
static uint32_t smcount, highcf, lowcf, waves, overshoot, calcbars;

static fftwf_plan pl, pr;
fftwf_complex *outl, *outr;

bool senseLow;

void separate_freq_bands(fftwf_complex *out, int bars, int channel, double sens, double ignore, int fftsize) {
    int o,i;
    double y[fftsize / 2 + 1];
    double temp;

    // process: separate frequency bands
    for (o = 0; o < bars; o++) {
        peak[o] = 0;

        // process: get peaks
        for (i = lcf[o]; i <= hcf[o]; i++) {
            //getting r of compex
            y[i] = hypot(out[i][0], out[i][1]);
            peak[o] += y[i]; //adding upp band
        }

        peak[o] = peak[o] / (hcf[o]-lcf[o] + 1); //getting average
        temp = peak[o] * sens * k[o] / 800000; //multiplying with k and sens
        if (temp <= ignore) temp = 0;
        if (channel == 1) fl[o] = temp;
        else fr[o] = temp;
    }
}


void monstercat_filter(int bars, int waves, double monstercat, int *data) {
    int z;

    // process [smoothing]: monstercat-style "average"
    int m_y, de;
    if (waves > 0) {
        for (z = 0; z < bars; z++) { // waves
            data[z] = data[z] / 1.25;
            //if (f[z] < 1) f[z] = 1;
            for (m_y = z - 1; m_y >= 0; m_y--) {
                de = z - m_y;
                data[m_y] = max(data[z] - pow(de, 2), data[m_y]);
            }
            for (m_y = z + 1; m_y < bars; m_y++) {
                de = m_y - z;
                data[m_y] = max(data[z] - pow(de, 2), data[m_y]);
            }
        }
    } else if (monstercat > 0) {
        for (z = 0; z < bars; z++) {
            //if (f[z] < 1)f[z] = 1;
            for (m_y = z - 1; m_y >= 0; m_y--) {
                de = z - m_y;
                data[m_y] = max(data[z] / pow(monstercat, de), data[m_y]);
            }
            for (m_y = z + 1; m_y < bars; m_y++) {
                de = m_y - z;
                data[m_y] = max(data[z] / pow(monstercat, de), data[m_y]);
            }
        }
    }
}

EXP_FUNC int xavaFilterInit(XAVA *xava) {
    XAVA_AUDIO *audio = &xava->audio;
    XAVA_CONFIG *p  = &xava->conf;

    // fft: planning to rock
    CALLOC_SELF(outl, audio->fftsize/2+1);
    pl = fftwf_plan_dft_r2c_1d(audio->fftsize, audio->audio_out_l, outl, FFTW_MEASURE);

    if(p->stereo) {
        CALLOC_SELF(outr, audio->fftsize/2+1);
        pr = fftwf_plan_dft_r2c_1d(audio->fftsize, audio->audio_out_r, outr, FFTW_MEASURE);
    }

    xavaBailCondition(highcf > audio->rate / 2,
            "Higher cutoff cannot be higher than the sample rate / 2");

    MALLOC_SELF(fc, xava->bars);
    MALLOC_SELF(fre, xava->bars);
    MALLOC_SELF(fpeak, xava->bars);
    MALLOC_SELF(k, xava->bars);
    MALLOC_SELF(f, xava->bars);
    MALLOC_SELF(lcf, xava->bars);
    MALLOC_SELF(hcf, xava->bars);
    MALLOC_SELF(fmem, xava->bars);
    MALLOC_SELF(flast, xava->bars);
    MALLOC_SELF(flastd, xava->bars);
    MALLOC_SELF(fall, xava->bars);
    MALLOC_SELF(fl, xava->bars);
    MALLOC_SELF(fr, xava->bars);
    MALLOC_SELF(peak, xava->bars);

    return 0;
}

EXP_FUNC void xavaFilterApply(XAVA *xava) {
    XAVA_AUDIO *audio = &xava->audio;
    XAVA_CONFIG *p  = &xava->conf;

    // if fc is not cleared that means that the variables are not initialized
    REALLOC_SELF(fc,xava->bars);
    REALLOC_SELF(fre,xava->bars);
    REALLOC_SELF(fpeak,xava->bars);
    REALLOC_SELF(k,xava->bars);
    REALLOC_SELF(f,xava->bars);
    REALLOC_SELF(lcf,xava->bars);
    REALLOC_SELF(hcf,xava->bars);
    REALLOC_SELF(fmem,xava->bars);
    REALLOC_SELF(flast,xava->bars);
    REALLOC_SELF(flastd,xava->bars);
    REALLOC_SELF(fall,xava->bars);
    REALLOC_SELF(fl,xava->bars);
    REALLOC_SELF(fr,xava->bars);
    REALLOC_SELF(peak,xava->bars+1);

    // oddoneout only works if the number of bars is odd, go figure
    if(oddoneout) {
        if (!(xava->bars%2))
            xava->bars--;
    }

    // update pointers for the main handle
    xava->f  = f;
    xava->fl = flastd;

    for (int i = 0; i < xava->bars; i++) {
        flast[i] = 0;
        flastd[i] = 0;
        fall[i] = 0;
        fpeak[i] = 0;
        fmem[i] = 0;
        f[i] = 0;
        lcf[i] = 0;
        hcf[i] = 0;
        fl[i] = 0;
        fr[i] = 0;
        fc[i] = .0;
        fre[i] = .0;
        fpeak[i] = .0;
        k[i] = .0;
    }

    // process [smoothing]: calculate gravity
    g = gravity * ((float)(xava->inner.h-1) / 2160) * (60 / (float)p->framerate);

    // checks if there is stil extra room, will use this to center
    xava->rest = (xava->inner.w - xava->bars * p->bw - xava->bars * p->bs + p->bs) / 2;
    if(xava->rest < 0)
        xava->rest = 0;

    if (p->stereo) xava->bars = xava->bars / 2; // in stereo onle half number of bars per channel

    if ((smcount > 0) && (xava->bars > 0)) {
        smh = (double)(((double)smcount)/((double)xava->bars));
    }

    // since oddoneout requires every odd bar, why not split them in half?
    calcbars = oddoneout ? xava->bars/2+1 : xava->bars;

    // frequency constant that we'll use for logarithmic progression of frequencies
    double freqconst = log(highcf-lowcf)/log(pow(calcbars, logScale));
    //freqconst = -2;

    // process: calculate cutoff frequencies
    int n;
    for (n=0; n < calcbars; n++) {
        fc[n] = pow(powf(n, (logScale-1.0)*((double)n+1.0)/((double)calcbars)+1.0),
                         freqconst)+lowcf;
        fre[n] = fc[n] / (audio->rate / 2.0);
        // Remember nyquist!, pr my calculations this should be rate/2
        // and  nyquist freq in M/2 but testing shows it is not...
        // or maybe the nq freq is in M/4

        //lfc stores the lower cut frequency foo each bar in the fft out buffer
        lcf[n] = floor(fre[n] * (audio->fftsize/2.0));

        if (n != 0) {
            //hfc holds the high cut frequency for each bar

            // I know it's not precise, but neither are integers
            // You can see why in https://github.com/nikp123/xava/issues/29
            // I did reverse the "next_bar_lcf-1" change
            hcf[n-1] = lcf[n];
        }
    }
    hcf[n-1] = highcf*audio->fftsize/audio->rate;

    // process: weigh signal to frequencies height and EQ
    for (n = 0; n < calcbars; n++) {
        k[n] = pow(fc[n], eqBalance);
        k[n] *= (float)(xava->inner.h-1) / 100;
        k[n] *= smooth[(int)floor(((double)n) * smh)];
    }

    if (p->stereo)
        xava->bars = xava->bars * 2;
}

EXP_FUNC void xavaFilterLoop(XAVA *xava) {
    XAVA_AUDIO *audio = &xava->audio;
    XAVA_CONFIG *p  = &xava->conf;
    int i;

    // process: execute FFT and sort frequency bands
    if (p->stereo) {
        fftwf_execute(pl);
        fftwf_execute(pr);

        separate_freq_bands(outl, calcbars, 1, p->sens, ignore, audio->fftsize);
        separate_freq_bands(outr, calcbars, 2, p->sens, ignore, audio->fftsize);
    } else {
        fftwf_execute(pl);
        separate_freq_bands(outl, calcbars, 1, p->sens, ignore, audio->fftsize);
    }

    if(oddoneout) {
        for(int i=xava->bars/2; i>0; i--) {
            fl[i*2-1+xava->bars%2]=fl[i];
            fl[i]=0;
        }
    }

    // process [smoothing]
    if (monstercat) {
        if (p->stereo) {
            monstercat_filter(xava->bars / 2, waves, monstercat, fl);
            monstercat_filter(xava->bars / 2, waves, monstercat, fr);
        } else {
            monstercat_filter(calcbars, waves, monstercat, fl);
        }
    }

    //preperaing signal for drawing
    for (i=0; i<xava->bars; i++) {
        if (p->stereo) {
            if (i < xava->bars / 2) {
                f[i] = fl[xava->bars / 2 - i - 1];
            } else {
                f[i] = fr[i - xava->bars / 2];
            }
        } else {
            f[i] = fl[i];
        }
    }


    // process [smoothing]: falloff
    if (g > 0) {
        for (i = 0; i < xava->bars; i++) {
            if (f[i] < flast[i]) {
                f[i] = fpeak[i] - (g * fall[i] * fall[i]);
                fall[i]++;
            } else  {
                fpeak[i] = f[i];
                fall[i] = 0;
            }
            flast[i] = f[i];
        }
    }

    // process [smoothing]: integral
    if (integral > 0) {
        for (i=0; i<xava->bars; i++) {
            f[i] = fmem[i] * integral + f[i];
            fmem[i] = f[i];

            int diff = xava->inner.h - f[i];
            if (diff < 0) diff = 0;
            double div = 1.0 / (double)(diff + 1);
            //f[o] = f[o] - pow(div, 10) * (height + 1);
            fmem[i] = fmem[i] * (1 - div / 20);
        }
    }

    // process [oddoneout]
    if(oddoneout) {
        for(i=1; i<xava->bars-1; i+=2) {
            f[i] = f[i+1]/2 + f[i-1]/2;
        }
        for(i=xava->bars-3; i>1; i-=2) {
            int sum = f[i+1]/2 + f[i-1]/2;
            if(sum>f[i]) f[i] = sum;
        }
    }

    senseLow = true;

    // automatic sens adjustment
    if (p->autosens&&(!xava->pauseRendering)) {
        // don't adjust on complete silence
        // as when switching tracks for example
        for (i=0; i<xava->bars; i++) {
            if (f[i] > (int)((xava->inner.h-1)*(100+overshoot)/100) ) {
                senseLow = false;
                p->sens *= 0.985;
                break;
            }
        }
        if (senseLow) {
            p->sens *= 1.001;
        }
    }
}

EXP_FUNC void xavaFilterCleanup(XAVA *xava) {
    free(outl);
    free(outr);
    fftwf_destroy_plan(pl);
    fftwf_destroy_plan(pr);
    fftwf_cleanup();

    free(smooth);

    // honestly even I don't know what these mean
    // but for the meantime, they are moved here and I won't touch 'em
    free(fc);
    free(fre);
    free(fpeak);
    free(k);
    free(f);
    free(lcf);
    free(hcf);
    free(fmem);
    free(flast);
    free(flastd);
    free(fall);
    free(fl);
    free(fr);
    free(peak);
}

EXP_FUNC void xavaFilterLoadConfig(XAVA *xava) {
    XAVA_CONFIG *p = &xava->conf;
    xava_config_source config = xava->default_config.config;

    p->sens     = xavaConfigGetDouble(config, "filter", "sensitivity", 100.0) *
        XAVA_PREDEFINED_SENS_VALUE; // check shared.h for details
    p->autosens = xavaConfigGetBool  (config, "filter", "autosens", 1);
    overshoot   = xavaConfigGetInt   (config, "filter", "overshoot", 0);
    lowcf       = xavaConfigGetInt   (config, "filter", "lower_cutoff_freq", 26);
    highcf      = xavaConfigGetInt   (config, "filter", "higher_cutoff_freq", 15000);

    monstercat = xavaConfigGetDouble(config, "filter", "monstercat", 0.0) * 1.5;
    waves      = xavaConfigGetInt   (config, "filter", "waves", 0);
    integral   = xavaConfigGetDouble(config, "filter", "integral", 85);
    gravity    = xavaConfigGetDouble(config, "filter", "gravity", 100) * 50.0;
    ignore     = xavaConfigGetDouble(config, "filter", "ignore", 0);
    logScale   = xavaConfigGetDouble(config, "filter", "log", 1.55);
    oddoneout  = xavaConfigGetBool  (config, "filter", "oddoneout", 1);
    eqBalance  = xavaConfigGetDouble(config, "filter", "eq_balance", 0.67);

    // read & validate: eq
    smcount = xavaConfigGetKeyNumber(config, "eq");
    if (smcount > 0) {
        MALLOC_SELF(smooth, smcount);
        char **keys = xavaConfigGetKeys(config, "eq");
        for (int sk = 0; sk < smcount; sk++) {
            smooth[sk] = xavaConfigGetDouble(config, "eq", keys[sk], 1);
        }
    } else {
        smcount = 64; //back to the default one
        MALLOC_SELF(smooth, smcount);
        for(int i=0; i<64; i++) smooth[i]=1.0f;
    }

    // validate: gravity
    gravity = gravity / 100;
    if (gravity < 0) {
        xavaWarn("Gravity cannot be below 0");
        gravity = 0;
    }

    // validate: oddoneout
    xavaBailCondition((p->stereo&&oddoneout),
            "'oddoneout' and stereo channels do not work together!");

    // validate: integral
    integral = integral / 100;
    if (integral < 0) {
        xavaWarn("Integral cannot be below 0");
        integral = 0;
    } else if (integral > 1) {
        xavaWarn("Integral cannot be above 100");
        integral = 1;
    }

    // validate: cutoff
    if (lowcf == 0 ) lowcf++;
    xavaBailCondition(lowcf > highcf,
            "Lower frequency cutoff cannot be higher than the higher cutoff\n");
}
