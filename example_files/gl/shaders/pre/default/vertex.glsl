#version 420 core

#ifdef GL_ES
	precision mediump float;
#endif

// input vertex
in vec4 fft_bars;

void main() {
	gl_Position = fft_bars;
}
