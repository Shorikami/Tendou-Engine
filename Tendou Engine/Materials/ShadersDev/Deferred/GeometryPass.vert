#version 450

layout(location = 0) in vec3 aPos;
layout(location = 1) in vec3 aNormal;
layout(location = 2) in vec2 aTexCoord;
layout(location = 3) in vec3 aColor;

layout(location = 0) out vec3 outPos;
layout(location = 1) out vec3 outNorm;
layout(location = 2) out vec2 outTexCoord;
layout(location = 3) out vec3 outColor;

layout(set = 0, binding = 0) uniform WorldUBO
{
  mat4 proj;
  mat4 view;
  vec2 nearFar;
} worldUBO;

layout(push_constant) uniform Push
{
	mat4 modelMatrix; // projection * view * model
	mat4 normalMatrix;
} push;


void main()
{
	vec4 viewPos = push.modelMatrix * vec4(aPos, 1.0);
	gl_Position = worldUBO.proj * worldUBO.view * viewPos;
	
	outPos = viewPos.xyz;
	outNorm = mat3(transpose(inverse(push.modelMatrix))) * aNormal;
	outTexCoord = aTexCoord;
	outColor = aColor;
}