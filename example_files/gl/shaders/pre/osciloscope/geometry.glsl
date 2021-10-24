#version 420 core

layout (lines) in;
layout (line_strip, max_vertices = 2) out;

out float lenght; 

void main() {
	gl_Position = vec4(gl_in[0].gl_Position.xy/32768.0, -1.0, 1.0);
	EmitVertex();
	lenght = distance(gl_in[0].gl_Position.xy, gl_in[1].gl_Position.xy);
	gl_Position = vec4(gl_in[1].gl_Position.xy/32768.0, -1.0, 1.0);
	EmitVertex();
	lenght = distance(gl_in[0].gl_Position.xy, gl_in[1].gl_Position.xy);
	EndPrimitive();
}
