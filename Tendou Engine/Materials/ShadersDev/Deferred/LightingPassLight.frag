#version 450

layout (binding = 1) uniform sampler2D gPos;
layout (binding = 2) uniform sampler2D gNorm;
layout (binding = 3) uniform sampler2D gAlbedo;

layout(location = 0) in vec3 aPos;
layout(location = 1) in vec3 aNorm;
layout(location = 2) in vec2 aTexCoord;

layout (location = 0) out vec4 fragColor;

layout(push_constant) uniform Push
{
	mat4 modelMatrix; // projection * view * model
	vec4 pos;
	vec3 color;
	float range;
} push;

void main()
{
	vec3 fragPos = texture(gPos, gl_FragCoord.xy / vec2(1280, 720)).rgb;
	vec3 normal = texture(gNorm, gl_FragCoord.xy / vec2(1280, 720)).rgb;
	vec4 albedo = texture(gAlbedo, gl_FragCoord.xy / vec2(1280, 720));
	
	vec3 L = push.pos.xyz - fragPos;
	float dist = length(L);
	
	//vec3 V = normalize(lightPass.eyePos.xyz - fragPos);
	
	if (dist < 100.0f)
	{
		float ambIntensity = 0.5f;
		vec3 finalColor = albedo.rgb * ambIntensity;
		L = normalize(L);
		
		float att = push.range / (pow(dist, 2.0f) + 1.0f);
		
		vec3 N = normalize(normal);
		float NdotL = max(0.0f, dot(N, L));
		vec3 diff = push.color * albedo.rgb * NdotL;
		
		//vec3 R = reflect(-L, N);
		//float NdotR = max(0.0f, dot(R, V));
		//vec3 spec = lightPass.lights[i].color * albedo.a * pow(NdotR, 32.0f);
		//
		//finalColor += (diff + spec) * att;
		
		fragColor += vec4(0.5f);
	}
	else
	{
		fragColor += vec4(vec3(0.0f), 1.0f);
	}
}