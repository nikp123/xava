#ifndef H_CONFIG
#define H_CONFIG
	#include "module.h"

	extern struct config_params {
		char *color, *bcolor, **gradient_colors, *shadow_color;
		double monstercat, integral, gravity, ignore, sens, logScale, logBegin, logEnd,
	 eqBalance, foreground_opacity, background_opacity; 
		unsigned int lowcf, highcf, shdw, shdw_col, inputsize, fftsize, gradients, 
	 bgcol, col;
		double *smooth;
		int smcount, autobars, stereo, vsync, fixedbars, framerate, bw, bs,
	 autosens, overshoot, waves, w, h;
		char *winA;
		int wx, wy;
		_Bool oddoneout, fullF, transF, borderF, bottomF, interactF, taskbarF;
		XAVAMODULE *inputModule, *outputModule;
		void* (*xavaInput)(void*);
		void (*xavaInputHandleConfiguration)(void *, void *);
		int (*xavaInitOutput)(void);
		void (*xavaOutputClear)(void);
		int (*xavaOutputApply)(void);
		int (*xavaOutputHandleInput)(void);
		void (*xavaOutputDraw)(int, int, int*, int*);
		void (*xavaOutputCleanup)(void);
	} p;

	extern struct state_params {
		_Bool pauseRendering;
	} s;

	void load_config(char *configPath, void* p);
	void clean_config();
#endif
