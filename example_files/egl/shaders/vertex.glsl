attribute vec4 pos;
attribute vec4 color;
varying vec4 v_color;

void main() {
	gl_Position = pos;
	v_color = color;
}
