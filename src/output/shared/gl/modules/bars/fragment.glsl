#version 420 core

// color passed from the host
uniform vec4 background_color;

// screen width and height
uniform vec2 resolution;

// offset to the bars in case they dont occupy the whole viewport
uniform vec2 bars_offset;

uniform float intensity;

in vec4 color;

layout(location=0) out vec4 FragColor;

void main() {
  FragColor = color;
}

