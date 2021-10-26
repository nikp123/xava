#version 420 core

in vec2 texCoord;

uniform sampler2D color_texture;
uniform sampler2D depth_texture;

uniform vec4 background_color;

uniform float intensity;

layout(location=0) out vec4 FragColor;

vec4 correctForAlphaBlend(vec4 color) {
	return vec4(color.rgb*color.a, color.a);
}

vec4 append_color_properly(vec4 source, vec4 target) {
	target.a    = target.a > source.a ? target.a : source.a;
	target.rgb += source.rgb*target.a;
	return target;
}

void main() {
	vec4 depth = texture(depth_texture, texCoord);

	// test if infinite
	FragColor = append_color_properly(
		texture(color_texture, texCoord),
		vec4(background_color.rgb, background_color.a*(1.0-intensity)));

	//FragColor = correctForAlphaBlend(FragColor);
}

