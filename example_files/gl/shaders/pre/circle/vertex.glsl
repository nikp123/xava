#version 420 core

// input vertex
in vec4 fft_bars;

void main() {
	gl_Position = fft_bars;
}
