#version 450
#extension GL_ARB_separate_shader_objects : enable

layout( binding = 3) uniform sampler2D texSampler;

layout(location = 0) in vec3 inColor;
//layout(location = 1) in vec2 inTexCoord;
layout(location = 0) out vec4 outColor;


void main() {

	//outColor = texture(texSampler, inTexCoord);
	outColor = vec4(inColor, 1);
}