#version 450

layout(location = 0) out vec4 outColor;

layout(location = 0) in vec3 fragColor;
layout(location = 1) in vec3 fragPosWorld;
layout(location = 2) in vec3 fragNormalWorld;
layout(location = 3) in vec2 texCoords;

layout(set = 0, binding = 0) uniform WorldUBO
{
  mat4 proj;
  mat4 view;
  vec2 nearFar;
};

layout(set = 0, binding = 1) uniform LightsUBO
{
  vec4 lightPos[16];
  vec4 lightColor[16];
  vec4 lightDir[16];
  
  vec4 eyePos;
  vec4 emissive;
  vec4 globalAmbient;
  vec4 coefficients;

  vec4 fogColor;
  
  vec4 specular[16];
  vec4 ambient[16];
  vec4 diffuse[16];
  
  //// x = inner, y = outer, z = falloff, w = type
  vec4 lightInfo[16];
  
  vec4 modes;
  
  vec3 attenuation;
  int numLights;
} ubLights;

layout(set = 0, binding = 2) uniform sampler2D tex;

layout(push_constant) uniform Push
{
	mat4 modelMatrix; // projection * view * model
	mat4 normalMatrix;
} push;

float attValue(float c1, float c2, float c3, float dist)
{
	return min(1.0 / (c1 + c2 * dist + c3 * dist * dist), 1.0);
}

float fogValue(float n, float f, float v)
{
	return (f - v) / (f - n);
}

vec3 LightCalc(int id)
{
	vec3 surfaceNorm = normalize(fragNormalWorld);
	vec3 lightDir = normalize(ubLights.lightPos[id].xyz - fragPosWorld);
	//float att = 1.0f / dot(lightDir, lightDir);
	
	//vec3 lightColor = ubLights.lightColor[id].xyz * ubLights.lightColor[id].w * att;
	//vec3 amb = ubLights.ambient[id].xyz * ubLights.ambient[id].w;
	vec3 amb = ubLights.coefficients.x * ubLights.ambient[id].xyz;
	
	float nDotL = max(dot(surfaceNorm, lightDir), 0.0f);
	vec3 diff = ubLights.coefficients.y * nDotL * ubLights.diffuse[id].xyz;
	//vec4 diff = ubLights.diffuse[id] * texture(tex, texCoords);
	
	// specular lighting (blinn)
	vec3 viewDir = normalize(ubLights.eyePos.xyz - fragPosWorld);
	vec3 halfVec = normalize(lightDir + viewDir);
	float blinnTerm = dot(surfaceNorm, halfVec);
	blinnTerm = clamp(blinnTerm, 0, 1);
	blinnTerm = pow(blinnTerm, 255.0f);
	
	//vec3 reflectDir = reflect(-lightDir, surfaceNorm);
	//float sp = pow(max(dot(viewDir, reflectDir), 0.0f), 32);
	
	vec3 spec = ubLights.coefficients.z * blinnTerm * ubLights.specular[id].xyz;
	
	// attenuation
	float dist = length(ubLights.lightPos[id].xyz - fragPosWorld);
	float att = attValue(ubLights.attenuation.x, ubLights.attenuation.y, ubLights.attenuation.z, dist);
	
	return att* amb + att * (diff + spec);
}

void main()
{
	vec3 em = ubLights.emissive.xyz;
	
	// REMEMBER: PLEASE DON'T FORGET TO INITIALIZE VARIABLES
	// OR IT WILL BE GARBAGE!!! THIS ISN'T LIKE OPENGL!!!!!
	vec3 local = vec3(0.0f);
	
	for (int i = 0; i < ubLights.numLights; ++i)
	{
		local += LightCalc(i);
	}
	
	outColor = vec4(local, 1.0f);
}