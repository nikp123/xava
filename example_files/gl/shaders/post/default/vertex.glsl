#ifdef GL_ES
	precision mediump float;
#endif

// input vertex and texture coordinate
attribute vec4 a_position;
attribute vec2 a_texCoord;

// output texture map coordinate
varying vec2 v_texCoord;

void main() {
	gl_Position = a_position;
	v_texCoord = a_texCoord;
} 
