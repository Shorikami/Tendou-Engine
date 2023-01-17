#version 450

layout (location = 0) out vec3 gPosition;
layout (location = 1) out vec3 gNormal;
layout (location = 2) out vec2 gUVs;
layout (location = 3) out vec3 gDepth;

layout(location = 0) in vec3 inPos;
layout(location = 1) in vec3 inNorm;
layout(location = 2) in vec2 inTexCoords;
layout(location = 3) in vec3 inColor;

void main()
{
	gPosition = inPos;
    gNormal = normalize(inNorm);

    gUVs = inTexCoords;
    gDepth = vec3(length(inPos));
}