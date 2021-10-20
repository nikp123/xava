#version 420 core

in vec2 texCoord;

uniform sampler2D color_texture;
uniform sampler2D depth_texture;

vec4 shadow_color  = vec4(0.0, 0.0, 0.0, 1.0);
vec2 shadow_offset = vec2(-0.005, 0.01);

uniform vec4 background_color;

layout(location=0) out vec4 FragColor;

// Credit: https://github.com/Jam3/glsl-fast-gaussian-blur/blob/master/5.glsl
vec4 blur5(sampler2D image, vec2 uv, vec2 resolution, vec2 direction) {
	vec4 color = vec4(0.0);
	vec2 off1 = vec2(1.3333333333333333) * direction;
	color += texture(image, uv) * 0.29411764705882354;
	color += texture(image, uv + (off1 / resolution)) * 0.35294117647058826;
	color += texture(image, uv - (off1 / resolution)) * 0.35294117647058826;
	return color;
}

vec4 correctForAlphaBlend(vec4 color) {
	return vec4(color.rgb*color.a, color.a);
}

void main() {
	vec4 depth = texture(depth_texture, texCoord);

	// test if infinite
	if(depth.r == 1.0) {
		float color =  1.0 - blur5(depth_texture, shadow_offset+texCoord, vec2(2.0, 2.0), shadow_offset).r;
		FragColor   =  mix(background_color, shadow_color, color);
	} else {
		FragColor = texture(color_texture, texCoord);
	}

	FragColor = correctForAlphaBlend(FragColor);
}

