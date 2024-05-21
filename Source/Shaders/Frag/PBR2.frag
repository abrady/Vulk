#version 450

#include "common.glsl"
#include "lighting.frag"

layout(binding = VulkShaderBinding_TextureSampler) uniform sampler2D albedoMap;
layout(binding = VulkShaderBinding_DisplacementSampler) uniform sampler2D displacementMap; // TODO: Implement displacement mapping
layout(binding = VulkShaderBinding_RoughnessSampler) uniform sampler2D roughnessMap;
layout(binding = VulkShaderBinding_AmbientOcclusionSampler) uniform sampler2D aoMap; //TODO: double check we're sampling this correctly
layout(binding = VulkShaderBinding_NormalSampler) uniform sampler2D normalMap;
layout(binding = VulkShaderBinding_MetallicSampler) uniform sampler2D metallicMap;
layout(binding = VulkShaderBinding_ShadowMapSampler) uniform sampler2D shadowSampler;

EYEPOS_UBO(eyePosUBO);

layout(binding = VulkShaderBinding_Lights) uniform LightBuf { 
    PointLight lights[VulkLights_NumLights]; 
} lightsBuf;

layout (std140, binding = VulkShaderBinding_PBRDebugUBO) uniform PBRDebugUBO {
    uint isMetallic;      // 4 bytes
    float roughness;      // 4 bytes, follows directly because it's also 4-byte aligned
    bool diffuse;         // 4 bytes in GLSL
    bool specular;        // 4 bytes in GLSL
} PBRDebug;

layout (std140, binding = VulkShaderBinding_GlobalConstantsUBO) uniform GlobalConstantsUBO {
	vec2 iResolution;
} globalConstants;


layout(location = VulkShaderLocation_Pos) in vec3 inPos;
layout(location = VulkShaderLocation_Normal) in vec3 inNormal;
layout(location = VulkShaderLocation_Tangent) in vec3 inTangent;
layout(location = VulkShaderLocation_Bitangent) in vec3 inBitangent;
layout(location = VulkShaderLocation_TexCoord) in vec2 inTexCoord;

layout(location = 0) out vec4 outColor;

// G(x) = 2(n.x)/(n.x+sqrt(α²+(1-α²)(n.x)²))
// where x is both l and v and you take the max of the two?
// NOTE: this is the Smith Joint Approximation for GGX, there are other faster ones
// e.g. unreal does *not* use this
float GSmithJoint(vec3 n, vec3 x, float a2) {
	float nx = saturate(dot(n, x));
	return 2.0 * nx / (nx + sqrt(a2 + (1.0 - a2) * nx * nx));
}

vec3 PBRForLight(PointLight light, vec3 pos, vec3 albedo, float metallic, float roughness, vec3 N) {
	vec3 lightDir = normalize(light.pos - pos);
	vec3 viewDir = normalize(eyePosUBO.eyePos - pos);
	vec3 halfVec = normalize(viewDir + lightDir);
	float ndotl = saturate(dot(N, lightDir));
    float ndotv = saturate(dot(N, viewDir));
    float ndoth = saturate(dot(N, halfVec));
    float ndoth2 = ndoth * ndoth;

	// Cook-Torrence specular
	// D(h)G(l,v)F(v,h)/4(n.l)(n.v)

	// D(h) = α²/π((n.h)²(α²-1)+1)²
	float roughness2 = roughness * roughness;
	float d = ndoth2 * (roughness2 - 1.0) + 1.0;
	float D = roughness2 / (PI * d * d);

	// G(l,v) = G(l)G(v)
	float G = GSmithJoint(N, lightDir, roughness2) * GSmithJoint(N, viewDir, roughness2);

	// F(v,h) = F0 + (1-F0)(1-v.h)⁵  - schlick approximation
	vec3 F0 = mix(vec3(0.04), albedo, metallic); // albedo is often used here for metallics because they have no diffuse.
	vec3 F = F0 + (1.0 - F0) * pow(1.0 - saturate(dot(viewDir, halfVec)), 5.0);

	// specular term all together
    vec3 specular = D * G * F / (4.0 * max(ndotl, 0.0001) * max(ndotv, 0.0001));

	// the lambertian term
	vec3 lambertian = albedo * ndotl;

	// Combine results
    vec3 kS = F;
    vec3 kD = vec3(1.0) - kS;
    kD *= 1.0 - metallic; // Only non-metals have diffuse reflection

    vec3 diffuse = lambertian * kD / PI;
    return (diffuse + specular) * light.color * ndotl;
}

void main() {
    vec3 albedo = texture(albedoMap, inTexCoord).rgb;
    float metallic = texture(metallicMap, inTexCoord).r;
    float roughness = texture(roughnessMap, inTexCoord).r;
    float ao = texture(aoMap, inTexCoord).r;
	vec3 N = sampleNormalMap(normalMap, inTexCoord, inNormal, inTangent, inBitangent);

	vec3 color = vec3(0.0);
	for (int i = 0; i < VulkLights_NumLights; i++) {
		color += PBRForLight(lightsBuf.lights[i], inPos, albedo, metallic, roughness, N);
	}

	vec3 ambientLightColor = vec3(0.1); // TODO: get this from somewhere
	vec3 ambient = ao * albedo * ambientLightColor;
	outColor = vec4(color + ambient, 1.0);
}

/*
// duplicate HLSL's saturate
float saturate(float x) {
	return clamp(x, 0.0, 1.0);
}

vec3 EnvRemap(vec3 c)
{
	return pow(2. * c, vec3(2.2));
}


// Main fragment shader function
void main() {
	// TODO: integrate these
	// Sample textures
    // vec3 albedo = texture(albedoMap, inTexCoord).rgb;
    // float metallic = texture(metallicMap, inTexCoord).r;
    // float roughness = texture(roughnessMap, inTexCoord).r;
    float ao = texture(aoMap, inTexCoord).r;
	vec3 N = sampleNormalMap(normalMap, inTexCoord, inNormal, inTangent, inBitangent);

	vec2 iResolution = globalConstants.iResolution;
	vec2 uv = inTexCoord.xy / iResolution.xy;
	vec2 q = inTexCoord.xy / iResolution.xy;
	vec2 p = -1. + 2. * q;
	p.x *= iResolution.x / iResolution.y;
	p *= 100.;

	vec3 color = vec3(1., .98, .94) * mix(1.0, 0.4, smoothstep(0., 1., saturate(abs(.5 - uv.y))));
	float vignette = q.x * q.y * (1.0 - q.x) * (1.0 - q.y);
	vignette = saturate(pow(32.0 * vignette, 0.05));
	color *= vignette;

	// 	DrawScene(color, p, s);
	// TODO: not actually using the lights we're passing in
	vec3 lightColor = vec3(2.);
	vec3 lightDir = normalize(vec3(.7, .9, -.2));

	// TODO: different metals
	vec3 baseColor = pow(vec3(0.74), vec3(2.2));
	vec3 diffuseColor = PBRDebug.isMetallic == 1. ? vec3(0.) : baseColor;
	vec3 specularColor = PBRDebug.isMetallic == 1. ? baseColor : vec3(0.02);
	float roughnessE = PBRDebug.roughness * PBRDebug.roughness;
	float roughnessL = max(.01, roughnessE);

	vec3 rayOrigin = vec3(0.0, .5, -3.5);
	vec3 rayDir = normalize(vec3(p.x, p.y, 2.0));
	// TODO: we may or may not want to duplicate this but it feels like a shadowmap
	// would work just fine.
	// float t = CastRay(rayOrigin, rayDir, localToWorld);
	// if (t <= 0) {
	// 	// shadow
	// 	float planeT = -(rayOrigin.y + 1.2) / rayDir.y;
	// 	if (planeT > 0.0)
	// 	{
	// 		vec3 p = rayOrigin + planeT * rayDir;

	// 		float radius = .7;
	// 		color *= 0.7 + 0.3 * smoothstep(0.0, 1.0, saturate(length(p + vec3(0.0, 1.0, -0.5)) - radius));
	// 	}
	// 	outColor = color;
	// 	return;
	// }

	//vec3 pos = rayOrigin + t * rayDir;
	// vec3 normal = SceneNormal(inPos, localToWorld);
	vec3 normal = N;
	vec3 viewDir = -rayDir;
	vec3 refl = reflect(rayDir, normal);

	vec3 diffuse = vec3(0.);
	vec3 specular = vec3(0.);

	vec3 halfVec = normalize(viewDir + lightDir);
	float vdoth = saturate(dot(viewDir, halfVec));
	float ndoth = saturate(dot(normal, halfVec));
	float ndotv = saturate(dot(normal, viewDir));
	float ndotl = saturate(dot(normal, lightDir));
	// TODO: replace this with a more powerful one
	// EnvBRDFApprox(specularColor, roughnessE, ndotv); 
	// see https://www.unrealengine.com/en-US/blog/physically-based-shading-on-mobile
	const vec4 c0 = vec4(-1, -0.0275, -0.572, 0.022);
	const vec4 c1 = vec4(1, 0.0425, 1.04, -0.04);
	vec4 r = roughnessE * c0 + c1;
	float a004 = min(r.x * r.x, exp2(-9.28 * ndotv)) * r.x + r.y;
	vec2 AB = vec2(-1.04, 1.04) * a004 + r.zw;
	vec3 envSpecularColor =  specularColor * AB.x + AB.y;

	// TODO: figure out what this is doing: literally just pow(2. * c, vec3(2.2));
	// vec3 env1 = EnvRemap(texture(iChannel2, refl).xyz);
	// vec3 env2 = EnvRemap(texture(iChannel1, refl).xyz);
	// vec3 env3 = EnvRemap(SHIrradiance(refl));
	// vec3 env = mix(env1, env2, saturate(roughnessE * 4.));
	// env = mix(env, env3, saturate((roughnessE - 0.25) / 0.75));
	// diffuse += diffuseColor * EnvRemap(SHIrradiance(normal));
	// specular += envSpecularColor * env;
	specular += envSpecularColor;
	diffuse += diffuseColor * lightColor * saturate(dot(normal, lightDir));

	float r2 = roughnessL * roughnessL;

	vec3 lightF = specularColor + (1. - specularColor) * pow((1. - vdoth), 5.); // FresnelTerm(specularColor, vdoth);

	// 	 DistributionTerm(roughnessL, ndoth)
	float d = (ndoth * r2 - ndoth) * ndoth + 1.0;
	float lightD = r2 / (d * d * PI);

	// VisibilityTerm(roughnessL, ndotv, ndotl);
	float gv = ndotl * sqrt(ndotv * (ndotv - ndotv * r2) + r2);
	float gl = ndotv * sqrt(ndotl * (ndotl - ndotl * r2) + r2);
	float lightV = 0.5 / max(gv + gl, 0.00001);

	specular += lightColor * lightF * (lightD * lightV * PI * ndotl);

	// for AO - using the texture above
	// SceneAO(pos, normal, localToWorld);
	diffuse *= ao;
	specular *= saturate(pow(ndotv + ao, roughnessE) - 1. + ao);

	color = vec3(0.); // diffuse + specular;
	if (PBRDebug.diffuse)
	{
		color += diffuse;
	}
	if (PBRDebug.specular)
	{
		color += specular;
	}
	// if (s.menuId == MENU_DISTR)
	// {
	// 	color = vec3(lightD);
	// }
	// if (s.menuId == MENU_FRESNEL)
	// {
	// 	color = envSpecularColor;
	// }
	// if (s.menuId == MENU_GEOMETRY)
	// {
	// 	color = vec3(lightV) * (4.0f * ndotv * ndotl);
	// }
	color = pow(color * .4, vec3(1. / 2.2));

    // Output final color
    outColor = vec4(color, 1.0);
}

*/