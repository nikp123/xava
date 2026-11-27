#version 420 core

// input vertex
in vec4 pos;
in vec4 color;

out vec4 vcolor;

uniform mat4 projection_matrix;

void main() {
	vcolor = color;
	gl_Position = pos*projection_matrix;
}
