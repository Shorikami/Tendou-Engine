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

void main()
{
	vec3 lightDir = ubLights.lightPos[0].xyz - fragPosWorld;
	float att = 1.0f / dot(lightDir, lightDir);
	
	vec3 lightColor = ubLights.lightColor[0].xyz * ubLights.lightColor[0].w * att;
	vec3 amb = ubLights.ambient[0].xyz * ubLights.ambient[0].w;
	
	vec3 diff = lightColor * max(dot(normalize(fragNormalWorld), normalize(lightDir)), 0.0f);
	
	outColor = vec4(diff, 1.0f) * texture(tex, texCoords);
}