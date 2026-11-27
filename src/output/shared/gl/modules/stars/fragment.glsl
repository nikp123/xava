#version 420 core

// color passed from the host
uniform vec4 foreground_color;

// color passed from the host
uniform vec4 background_color;

// screen width and height
uniform vec2 resolution;

uniform float intensity;

in vec4 vcolor;

layout(location=0) out vec4 FragColor;

void main() {
	FragColor = vcolor;
}
