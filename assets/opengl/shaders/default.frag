#version 110

uniform vec4 foreground_color;
uniform vec4 background_color;

uniform vec3 gradient_color[8];
uniform int gradients;

uniform vec4 shadow_color;
uniform int shadow_size;

uniform int window_width;
uniform int window_height;

uniform int rest;
uniform int bar_width;
uniform int bar_spacing;

uniform int bars;

// totally arbritary bar limitation due to GLSL not supporting dynamic arrays up until OGL 4.0
uniform int f[10000];

int x = int(gl_FragCoord.x);
int y = int(gl_FragCoord.y);
int a = x - rest;
int b = bar_width + bar_spacing;
int c = int(mod(float(a), float(b)));
int d = a / b;
int e = rest+b*(bars-1)+bar_width;

float progress;
int color1;
int color2;

vec3 finalColor;

vec4 mixColor(vec4 bg, vec4 fg) {
	return vec4(bg.rgb*(1.0-fg.a)+fg.rgb*(fg.a), max(bg.a,fg.a));
}

void drawBar(void) {
	if(gradients > 0) {
		progress=float(y-shadow_size)/float(window_height-2*shadow_size);
		progress*=float(gradients-1);

		color1 = int(floor(progress));
		color2 = int(ceil(progress));

		progress=mod(progress, 1.0);

		finalColor = gradient_color[color2]*progress + gradient_color[color1]*(1.0-progress);
		gl_FragColor = vec4(finalColor, foreground_color.a);
	} else {
		gl_FragColor = foreground_color;
	}
}

void drawBetweenBars(int dis) {
	float intensity = 1.0;

	// calculate for each side
	float one = float(shadow_size+dis-bar_spacing)/float(shadow_size);
	float two = float(shadow_size-dis)/float(shadow_size);

	// calculate shadow above bar
	if(y>f[d]+shadow_size) two*=float(f[d]+2*shadow_size-y)/float(shadow_size);
	if(y>f[d+1]+shadow_size) one*=float(f[d+1]+2*shadow_size-y)/float(shadow_size);

	intensity *= max(one, two);

	// check for shadow below bar
	if(y<shadow_size) 
		intensity*=float(y)/float(shadow_size);

	gl_FragColor = mixColor(background_color, vec4(shadow_color.rgb, shadow_color.a*intensity));
}

void drawAbove(int dis) {
	float intensity = float(shadow_size-dis+1)/float(shadow_size);
	gl_FragColor = mixColor(background_color, vec4(shadow_color.rgb, shadow_color.a*intensity));
}

void drawBelow(int dis) {
	float intensity = float(dis)/float(shadow_size);
	gl_FragColor = mixColor(background_color, vec4(shadow_color.rgb, shadow_color.a*intensity));
}

void drawOutOfBounds(int dis) {
	float intensity = float(dis)/float(shadow_size);

	// check for shadow below bar
	if(y<shadow_size) intensity*=float(y)/float(shadow_size);

	// calculate shadow above bar
	if(y>f[d]+shadow_size) intensity*=float(shadow_size*2-(y-f[d]))/float(shadow_size);
	if(y>f[d]+shadow_size*2) intensity = 0.0;
	gl_FragColor = mixColor(background_color, vec4(shadow_color.rgb, shadow_color.a*intensity));
}

void main( void ) {
	x = int(gl_FragCoord.x);
	y = int(gl_FragCoord.y);

	a = x - rest;
	b = bar_width + bar_spacing;
	c = int(mod(float(a), float(b)));
	d = a / b;
	e = rest+b*(bars-1)+bar_width;

	if(x < rest) {
		drawOutOfBounds(shadow_size+x-rest);
	} else if(x>e-1) {
		// TODO: check if the above code contains rounding errors
		drawOutOfBounds(e+shadow_size-x);
	} else {
		if(c < bar_width) {
			if(y<shadow_size) drawBelow(y);
			else if(y<f[d]+shadow_size) drawBar();
			else drawAbove(y-f[d]-shadow_size+1);
		} else drawBetweenBars(c-bar_width);
	}
}

