#version 450

layout(location = 0) in vec3 aPos;
layout(location = 1) in vec3 aNormal;
layout(location = 2) in vec2 aTexCoord;
layout(location = 3) in vec3 aColor;

layout(location = 0) out vec3 fragColor;
layout(location = 1) out vec3 fragPosWorld;
layout(location = 2) out vec3 fragNormalWorld;
layout(location = 3) out vec2 texCoords;

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
	vec4 viewPos = push.modelMatrix * vec4(aPos, 1.0);
	gl_Position = worldUBO.proj * worldUBO.view * viewPos;
	
	fragNormalWorld = normalize(mat3(push.normalMatrix) * aNormal);
	fragPosWorld = viewPos.xyz;
	fragColor = aColor;
	texCoords = aTexCoord;
}