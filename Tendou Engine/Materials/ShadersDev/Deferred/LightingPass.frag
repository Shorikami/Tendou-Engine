#version 450

layout (binding = 0) uniform sampler2D gPos;
layout (binding = 1) uniform sampler2D gNorm;
layout (binding = 2) uniform sampler2D gAlbedo;

layout (location = 0) in vec2 outTex;

layout (location = 0) out vec4 fragColor;

#define MAX_LIGHTS 10

struct Light
{
	vec4 pos;
	vec3 color;
	float radius;
};

layout (binding = 3) uniform LightPass
{
	Light lights[MAX_LIGHTS];
	vec4 eyePos;
	int displayTarget;
} lightPass;

void main()
{
	vec3 fragPos = texture(gPos, outTex).rgb;
	vec3 normal = texture(gNorm, outTex).rgb;
	vec4 albedo = texture(gAlbedo, outTex);
	
	if (lightPass.displayTarget > 0)
	{
		switch (lightPass.displayTarget)
		{
			case 1:
				fragColor = vec4(fragPos, 1.0f);
				break;
			case 2:
				fragColor = vec4(normal, 1.0f);
				break;
			case 3:
				fragColor = vec4(albedo.rgb, 1.0f);
				break;
			case 4:
				fragColor = vec4(albedo.aaa, 1.0f);
				break;
		}
	}
	
	float ambIntensity = 0.5f;
	
	vec3 finalColor = albedo.rgb * ambIntensity;
	
	for (int i = 0; i < MAX_LIGHTS; ++i)
	{
		vec3 L = lightPass.lights[i].pos.xyz - fragPos;
		float dist = length(L);
		
		vec3 V = normalize(lightPass.eyePos.xyz - fragPos);
		
		L = normalize(L);
		vec3 N = normalize(normal);
		float NdotL = max(0.0f, dot(N, L));
		vec3 diff = lightPass.lights[i].color * albedo.rgb * NdotL;
		
		vec3 R = reflect(-L, N);
		float NdotR = max(0.0f, dot(R, V));
		vec3 spec = lightPass.lights[i].color * albedo.a * pow(NdotR, 32.0f);
		
		finalColor += (diff + spec);
	}
	
	fragColor = vec4(finalColor, 1.0f);
}