struct config_params {
	char *color, *bcolor, *raw_target, *audio_source, *gradient_color_1, *gradient_color_2, *shadow_color, *data_format;
	char bar_delim, frame_delim, oddoneout;
	double monstercat, integral, gravity, ignore, sens, foreground_opacity, logScale; 
	unsigned int lowcf, highcf, shdw, shdw_col;
	double *smooth;
	int smcount, customEQ, im, om, col, bgcol, autobars, stereo, is_bin, ascii_range,
 bit_format, gradient, fixedbars, framerate, bw, bs, autosens, overshoot, waves,
 set_win_props, w, h, fftsize;
};
void load_config(char configPath[255], char supportedInput[255], void* p);
