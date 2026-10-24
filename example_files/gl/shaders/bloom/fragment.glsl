#version 420 core

// A very simple 1D blur applied in two passes because compute kernels are
// expensive. This won't render an accurate blur, but a good enough looking one.

// Based on: https://github.com/Experience-Monks/glsl-fast-gaussian-blur

in vec2 texCoord;

uniform sampler2D color_texture;
uniform sampler2D depth_texture;

uniform vec4 background_color;

uniform vec2 resolution;

uniform float intensity;

layout(location=0) out vec4 FragColor;

/**
 * User configurable variables for your 🌟 aesthetic 🌟 needs
**/

// Controls how large the blur would become
#define strenght 3.0
// Controls how large the audio intensity needs to be before showing the effect
#define cutoff   1.0
// Blur regardless of intensity
//#define SKIP_INTENSITY

vec4 blur13(sampler2D image, vec2 uv, vec2 resolution, vec2 direction) {
  vec4 color = vec4(0.0);
  vec2 off1 = vec2(1.411764705882353) * direction;
  vec2 off2 = vec2(3.2941176470588234) * direction;
  vec2 off3 = vec2(5.176470588235294) * direction;
  color += texture(image, uv) * 0.1964825501511404;
  color += texture(image, uv + (off1 / resolution)) * 0.2969069646728344;
  color += texture(image, uv - (off1 / resolution)) * 0.2969069646728344;
  color += texture(image, uv + (off2 / resolution)) * 0.09447039785044732;
  color += texture(image, uv - (off2 / resolution)) * 0.09447039785044732;
  color += texture(image, uv + (off3 / resolution)) * 0.010381362401148057;
  color += texture(image, uv - (off3 / resolution)) * 0.010381362401148057;
  return color;
}

void main() {
  #ifndef SKIP_INTENSITY
  vec2 horiz = vec2(max(0.0, strenght*intensity*4 - cutoff), 0.0);
  vec2 vert  = vec2(0.0, max(0.0, strenght*intensity*4 - cutoff));
  #else
  vec2 horiz = vec2(max(0.0, strenght), 0.0);
  vec2 vert  = vec2(0.0, max(0.0, strenght));
  #endif
  vec2 uv = vec2(gl_FragCoord.xy / resolution);

  // We're doing two blur passes because one blur is simply ugly,
  // plus it achieves a stylized retro effect
  FragColor = 
    (blur13(color_texture, uv, resolution, horiz) +
    blur13(color_texture, uv, resolution, vert))
    *
    texture(depth_texture, uv) +
    texture(color_texture, uv);
}

