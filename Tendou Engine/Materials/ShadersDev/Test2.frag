#version 450

layout(location = 0) out vec4 outColor;

layout(location = 0) in vec3 fragColor;
layout(location = 1) in vec3 fragPosWorld;
layout(location = 2) in vec3 fragNormalWorld;

layout(push_constant) uniform Push
{
	mat4 modelMatrix; // projection * view * model
	mat4 normalMatrix;
} push;

void main()
{
	outColor = vec4(1.0);
}