// color passed from the host
uniform vec4 foreground_color;

// color passed from the host
uniform vec4 background_color;

// screen width and height
uniform vec2 resolution;

uniform float intensity;

varying vec4 vcolor;

void main() {
	gl_FragColor.rgb += vcolor.rgb   * vcolor.a;
	gl_FragColor.rgb += gl_Color.rgb * gl_Color.a;
	gl_FragColor.rgb /= vcolor.a     + gl_Color.a;
	gl_FragColor.a    = max(vcolor.a, gl_Color.a);
}

