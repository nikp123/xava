// input vertex
attribute vec4 pos;
attribute vec4 color;

varying vec4 vcolor;

uniform mat4 projection_matrix;

void main() {
	gl_Position = pos*projection_matrix;
	vcolor = color;
}
