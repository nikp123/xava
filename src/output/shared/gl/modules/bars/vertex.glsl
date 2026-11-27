#version 420 core

// input vertex
in vec4 fft_bars;
in vec4 foreground_color;

out vec4 color;

uniform mat4 projection_matrix;

void main() {
  color = foreground_color;
  gl_Position = fft_bars*projection_matrix;
}
