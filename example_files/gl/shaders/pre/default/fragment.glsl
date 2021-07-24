#ifdef GL_ES
	precision mediump float;
#endif

// color determined by the vertex shader
varying vec4 v_color;

// number of secionts that the gradient is divided to
uniform float gradient_sections;

// each of the gradient colors
uniform vec4 gradient_color[8];

// screen width and height
uniform vec2 u_resolution;

void main() {
	if(gradient_sections > 0.0) {
		float across = (gl_FragCoord.y/u_resolution.y)*gradient_sections;
		int section = int(floor(across));
		float off = mod(across, 1.0);
		gl_FragColor = mix(gradient_color[section], gradient_color[section+1], off);
	} else {
		gl_FragColor = v_color;
	}
}

