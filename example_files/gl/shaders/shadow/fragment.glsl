#version 420 core

in vec2 texCoord;

uniform sampler2D color_texture;
uniform sampler2D depth_texture;

uniform vec2 resolution;

vec4 shadow_color  = vec4(0.0, 0.0, 0.0, 1.0);
vec2 shadow_offset, shadow_direction;

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

vec4 append_color_properly(vec4 source, vec4 target) {
	target.rgb += source.rgb*source.a;
	target.a    = max(source.a, target.a);
	return target;
}

void main() {
	// transfer background
	FragColor = texture(color_texture, texCoord);

	shadow_direction = vec2(-5.0, 5.0);
	shadow_offset = vec2(-5.0, 5.0)/resolution;

	// test if infinite
	vec4 depth = texture(depth_texture, texCoord);
	if(depth.r == 1.0) {
		float color =  1.0 - blur5(depth_texture, shadow_offset+texCoord,
				shadow_direction, shadow_offset).r;
		FragColor   =  mix(FragColor, shadow_color, color);
	}

	FragColor = correctForAlphaBlend(FragColor);
}

