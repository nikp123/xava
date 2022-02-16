#version 420 core

// color passed from the host
uniform vec4 foreground_color;

// color passed from the host
uniform vec4 background_color;

// screen width and height
uniform vec2 resolution;

uniform float intensity;

layout(location=0) out vec4 FragColor;

void main() {
	FragColor = foreground_color;
}

