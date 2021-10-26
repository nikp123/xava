#version 420 core

layout (points) in;
layout (triangle_strip, max_vertices = 8) out;

uniform float rest;
uniform float bar_width;
uniform float bar_spacing;
uniform float bar_count;

uniform vec2 resolution;

uniform mat4 projection_matrix;

vec2 rotate(vec2 v, float a) {
	float s = sin(a);
	float c = cos(a);
	mat2 m = mat2(c, -s, s, c);
	return m * v;
}

void displayCircleVert(float x, float y) {
	vec2 r = rotate(vec2(0.0, y), 6.28*x/resolution.x);
	gl_Position = vec4(r, -1.0, 1.0);
	gl_Position += vec4(resolution.xy/2.0, 0.0, 0.0);
	gl_Position *= projection_matrix;
	EmitVertex();
}

float height_buffer = resolution.y/8.0;

void main() {
	float index = gl_in[0].gl_Position.x;

	float position = rest + (bar_width+bar_spacing)*index;
	float position_end = position + bar_width;

	float height = gl_in[0].gl_Position.y;
	height /= (height_buffer+resolution.y)/resolution.y;
	height += height_buffer;
	height /= 2.0;

	displayCircleVert(position_end, height_buffer/2.0);
	displayCircleVert(position,     height_buffer/2.0);
	displayCircleVert(position_end, height);
	displayCircleVert(position,     height);
	EndPrimitive();
}
