#version 420 core

layout (points) in;
layout (triangle_strip, max_vertices = 12) out;

uniform float rest;
uniform float bar_width;
uniform float bar_spacing;
uniform float bar_count;

void main() {
	float index = gl_in[0].gl_Position.x;

	float position = -1.0 + rest + (bar_width+bar_spacing)*index;
	float position_end = position + bar_width;

	float height = gl_in[0].gl_Position.y * 2.0 - 1.0;

	if(index < 1.0) {
		// do shit

		float scaler = 100.0;

		gl_Position = vec4( scaler, -scaler, 1.0, scaler);
		EmitVertex();
		gl_Position = vec4(-scaler, -scaler, 1.0, scaler);
		EmitVertex();
		gl_Position = vec4( scaler,  scaler, 1.0, scaler);
		EmitVertex();
		gl_Position = vec4(-scaler,  scaler, 1.0, scaler);
		EmitVertex();
		EndPrimitive();

	}

	gl_Position = vec4(position_end, -1.0,   -1.0, 1.0);
	EmitVertex();
	gl_Position = vec4(position,     -1.0,   -1.0, 1.0);
	EmitVertex();
	gl_Position = vec4(position_end, height, -1.0, 1.0);
	EmitVertex();
	gl_Position = vec4(position,     height, -1.0, 1.0);
	EmitVertex();
	EndPrimitive();
}
