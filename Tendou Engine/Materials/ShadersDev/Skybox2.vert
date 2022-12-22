#version 450

layout(location = 0) in vec3 aPos;
layout(location = 1) in vec3 aNormal;
layout(location = 2) in vec2 aTexCoord;
layout(location = 3) in vec3 aColor;

layout(location = 0) out vec2 texCoords;
layout(location = 1) out vec3 fragPos;

layout(set = 0, binding = 0) uniform WorldUBO
{
	mat4 proj;
	mat4 view;
	vec2 nearFar;
} worldUBO;

layout(set = 0, binding = 5) uniform RenderUBO
{
	mat4 proj;
	mat4 view;
} captureUBO;

layout(push_constant) uniform Push
{
	mat4 modelMatrix;
	mat4 normalMatrix;
} push;

void main()
{
	vec4 viewPos = push.modelMatrix * vec4(aPos, 1.0);
	
	// TODO: View calculation is expensive. Do it on the CPU
	gl_Position = captureUBO.proj * mat4(mat3(captureUBO.view)) * viewPos;
	
	fragPos = aPos;
	texCoords = aTexCoord;
}