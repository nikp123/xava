struct config_params {
	char *color, *bcolor, *raw_target, *audio_source, **gradient_colors, *shadow_color,
 *data_format;
	char bar_delim, frame_delim, oddoneout;
	double monstercat, integral, gravity, ignore, sens, logScale, logBegin, logEnd, 
 eqBalance, foreground_opacity, background_opacity; 
	unsigned int lowcf, highcf, shdw, shdw_col, inputsize, fftsize, gradients, 
 bgcol, col;
	double *smooth;
	int smcount, im, om, autobars, stereo, is_bin, ascii_range, vsync,
 bit_format, fixedbars, framerate, bw, bs, autosens, overshoot, waves,
 w, h;
	char *winA;
	int wx, wy;
	_Bool fullF, transF, borderF, bottomF, winPropF, interactF, taskbarF;
} p;
void load_config(char configPath[255], char supportedInput[255], void* p);
