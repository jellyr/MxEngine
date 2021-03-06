#include "shader_utils.glsl"
EMBEDDED_SHADER(

struct DirLight
{
	mat4 transform;
	vec3 ambient;
	vec3 diffuse;
	vec3 specular;
	vec3 direction;
};

vec3 calcColorUnderDirLight(FragmentInfo fragment, vec3 reflectionColor, DirLight light, vec3 viewDir, int pcfDistance, vec4 fragLightSpace, sampler2D shadowMap)
{
	vec3 lightDir = normalize(light.direction);
	vec3 Hdir = normalize(lightDir + viewDir);
	float shadowFactor = calcShadowFactor2D(fragLightSpace, shadowMap, 0.005f, pcfDistance);

	float diffuseCoef = max(dot(lightDir, fragment.normal), 0.0f);
	float specularCoef = pow(max(dot(Hdir, fragment.normal), 0.0f), fragment.specularIntensity);

	vec3 ambientColor = fragment.albedo;
	vec3 diffuseColor = fragment.albedo * diffuseCoef;
	vec3 specularColor = vec3(specularCoef * fragment.specularFactor);

	ambientColor = ambientColor * light.ambient;
	diffuseColor = diffuseColor * light.diffuse;
	specularColor = specularColor * light.specular;
	reflectionColor = diffuseCoef * reflectionColor;

	return vec3(ambientColor + shadowFactor * (specularColor + mix(diffuseColor, reflectionColor, fragment.reflection)));
}

)