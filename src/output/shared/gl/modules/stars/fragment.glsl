// color passed from the host
uniform vec4 foreground_color;

// color passed from the host
uniform vec4 background_color;

// screen width and height
uniform vec2 resolution;

uniform float intensity;

varying vec4 vcolor;

void main() {
	gl_FragColor = vcolor;
}

