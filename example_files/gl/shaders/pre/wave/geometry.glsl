#version 420 core

layout (lines) in;
layout (line_strip, max_vertices = 4) out;

out float lenght; 

uniform float audio_rate;

void main() {
	float index = gl_in[0].gl_Position.y/audio_rate*2.0 - 1.0;
	float height = gl_in[0].gl_Position.x/32768.0;
	gl_Position = vec4(index, height, -1.0, 1.0);
	EmitVertex();

	index = gl_in[1].gl_Position.y/audio_rate*2.0 - 1.0;
	height = gl_in[1].gl_Position.x/32768.0;
	gl_Position = vec4(index, height, -1.0, 1.0);
	EmitVertex();
	EndPrimitive();
}
