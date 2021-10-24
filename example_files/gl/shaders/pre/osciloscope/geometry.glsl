#version 420 core

layout (lines) in;
layout (line_strip, max_vertices = 2) out;

out float lenght;

void main() {
	vec4 pos1 = vec4(gl_in[0].gl_Position.xy/32768.0, -1.0, 1.0);
	vec4 pos2 = vec4(gl_in[1].gl_Position.xy/32768.0, -1.0, 1.0);

	lenght = distance(pos1.xy, pos2.xy);
	gl_Position = pos1;
	EmitVertex();
	lenght = distance(pos1.xy, pos2.xy);
	gl_Position = pos2;
	EmitVertex();
	EndPrimitive();
}
