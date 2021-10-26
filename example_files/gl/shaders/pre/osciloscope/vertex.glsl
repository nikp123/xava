#version 420 core

// input vertex
in vec4 audio_data;

void main() {
	gl_Position = audio_data;
}
