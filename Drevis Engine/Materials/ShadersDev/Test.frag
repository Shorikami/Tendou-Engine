#version 450

layout(location = 0) out vec4 outColor;

layout(location = 0) in vec3 fragColor;
layout(location = 1) in vec3 fragPosWorld;
layout(location = 2) in vec3 fragNormalWorld;

layout(set = 0, binding = 0) uniform WorldUBO
{
	mat4 proj;
	mat4 view;
} worldUBO;

layout(set = 0, binding = 1) uniform LightUBO
{
	vec4 ambient;
	vec3 lightPos;
	vec4 lightColor;
} lightUBO;

layout(push_constant) uniform Push
{
	mat4 modelMatrix; // projection * view * model
	mat4 normalMatrix;
} push;

void main()
{
	vec3 lightDir = lightUBO.lightPos - fragPosWorld;
	float att = 1.0f / dot(lightDir, lightDir);
	
	vec3 lightColor = lightUBO.lightColor.xyz * lightUBO.lightColor.w * att;
	vec3 amb = lightUBO.ambient.xyz * lightUBO.ambient.w;
	
	vec3 diff = lightColor * max(dot(normalize(fragNormalWorld), normalize(lightDir)), 0.0f);
	
	outColor = vec4(fragColor, 1.0);
}