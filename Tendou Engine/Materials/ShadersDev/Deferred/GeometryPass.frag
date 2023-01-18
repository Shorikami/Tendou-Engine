#version 450

layout (binding = 1) uniform sampler2D samplerTex;

layout (location = 0) out vec3 gPosition;
layout (location = 1) out vec3 gNormal;
layout (location = 2) out vec4 gAlbedo;

layout(location = 0) in vec3 inPos;
layout(location = 1) in vec3 inNorm;
layout(location = 2) in vec2 inTexCoords;
layout(location = 3) in vec3 inColor;

void main()
{
	gPosition = inPos;
    gNormal = normalize(inNorm);
    gAlbedo = texture(samplerTex, inTexCoords);
}