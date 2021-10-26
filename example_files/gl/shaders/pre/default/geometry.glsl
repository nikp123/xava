#version 420 core

layout (points) in;
layout (triangle_strip, max_vertices = 8) out;

uniform float rest;
uniform float bar_width;
uniform float bar_spacing;
uniform float bar_count;

uniform mat4 projection_matrix;

void main() {
	float index = gl_in[0].gl_Position.x;

	float position = rest + (bar_width+bar_spacing)*index;
	float position_end = position + bar_width;

	float height = gl_in[0].gl_Position.y;


	gl_Position = vec4(position_end, 0.0,    -1.0, 1.0)*projection_matrix;
	EmitVertex();

	gl_Position = vec4(position,     0.0,    -1.0, 1.0)*projection_matrix;
	EmitVertex();
	gl_Position = vec4(position_end, height, -1.0, 1.0)*projection_matrix;
	EmitVertex();
	gl_Position = vec4(position,     height, -1.0, 1.0)*projection_matrix;
	EmitVertex();
	EndPrimitive();

	//gl_Position = vec4(1.0, 0.0, -1.0, 1.0);
	//EmitVertex();
	//gl_Position = vec4(0.0, 0.0, -1.0, 1.0);
	//EmitVertex();
	//gl_Position = vec4(1.0, 1.0, -1.0, 1.0);
	//EmitVertex();
	//gl_Position = vec4(0.0, 1.0, -1.0, 1.0);
	//EmitVertex();
	//EndPrimitive();
}
