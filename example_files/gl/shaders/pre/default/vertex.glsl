#ifdef GL_ES
	precision mediump float;
#endif

// input vertex
attribute vec4 pos;

// foreground color
uniform vec4 color;

// screen width and height
uniform vec2 u_resolution;

// projection matrix precalculated by XAVA
uniform mat4 projectionMatrix;

// output color used by the fragment shader
varying vec4 v_color;

void main() {
	gl_Position = pos;
	gl_Position *= projectionMatrix;
	v_color = color;
}
