#version 420 core

in vec2 texCoord;

uniform sampler2D color_texture;
uniform sampler2D depth_texture;

uniform vec4 background_color;

layout(location=0) out vec4 FragColor;

vec4 correctForAlphaBlend(vec4 color) {
	return vec4(color.rgb*color.a, color.a);
}

void main() {
	vec4 depth = texture(depth_texture, texCoord);

	// test if infinite
	FragColor = background_color;
	if(depth.r != 1.0) {
		FragColor += texture(color_texture, texCoord);
	}

	FragColor = correctForAlphaBlend(FragColor);
}

