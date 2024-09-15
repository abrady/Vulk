#version 450

#include "common.glsl"
#include "lighting.frag"

// G(x) = 2(n.x)/(n.x+sqrt(α²+(1-α²)(n.x)²))
// where x is both l and v and you take the max of the two?
// NOTE: this is the Smith Joint Approximation for GGX, there are other faster ones
// e.g. unreal does *not* use this
float GSmithJoint(vec3 n, vec3 x, float a2) {
	float nx = saturate(dot(n, x));
	return 2.0 * nx / (nx + sqrt(a2 + (1.0 - a2) * nx * nx));
}

vec3 PBRForLight(PointLight light, vec3 eyePos, vec3 pos, vec3 albedo, float metallic, float roughness, vec3 N) {
	vec3 lightDir = normalize(light.pos - pos);
	vec3 viewDir = normalize(eyePos - pos);
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
	float Gl = GSmithJoint(N, lightDir, roughness2);
	float Gv = GSmithJoint(N, viewDir, roughness2);
	float G =  Gl * Gv;

	// F(v,h) = F0 + (1-F0)(1-v.h)⁵  - schlick approximation
	vec3 F0 = mix(vec3(0.04), albedo, metallic); // albedo is often used here for metallics because they have no diffuse.
	vec3 F = F0 + (1.0 - F0) * pow(1.0 - saturate(dot(viewDir, halfVec)), 5.0);

	// specular term all together
    vec3 specular = (D * G * F) / (4.0 * max(ndotl, 0.0001) * max(ndotv, 0.0001));

	// the lambertian term
	vec3 lambertian = albedo * ndotl;

	// Combine results
    vec3 kS = F;
    vec3 kD = vec3(1.0) - kS;
    kD *= 1.0 - metallic; // Only non-metals have diffuse reflection

    vec3 diffuse = lambertian * kD / PI;
    return (diffuse + specular) * light.color * ndotl;
}

// layout(binding = Binding_DisplacementSampler) uniform sampler2D displacementMap; // TODO: Implement displacement mapping
// layout(binding = Binding_RoughnessSampler) uniform sampler2D roughnessMap;
// layout(binding = Binding_MetallicSampler) uniform sampler2D metallicMap;
// layout(binding = Binding_ShadowMapSampler) uniform sampler2D shadowSampler;

layout(binding = Binding_InvViewProjUBO) uniform InvViewProjUBO {
    mat4 invViewProj;
};

layout(binding = Binding_EyePos) uniform EyePos { 
    vec3 eyePos; 
} eyePosUBO;

layout(binding = Binding_Lights) uniform LightBuf { 
    PointLight lights[VulkLights_NumLights]; 
} lightsBuf;

layout (std140, binding = Binding_PBRDebugUBO) uniform PBRDebugUBO {
    uint isMetallic;      // 4 bytes
    float roughness;      // 4 bytes, follows directly because it's also 4-byte aligned
    bool diffuse;         // 4 bytes in GLSL
    bool specular;        // 4 bytes in GLSL
} PBRDebug;

layout(input_attachment_index = GBufInputAtmtIdx_Albedo,   binding = Binding_GBufAlbedo) uniform subpassInput albedoMap;
layout(input_attachment_index = GBufInputAtmtIdx_Normal,   binding = Binding_GBufNormal) uniform subpassInput normalMap;
layout(input_attachment_index = GBufInputAtmtIdx_Material, binding = Binding_GBufMaterial) uniform subpassInput materialMap;
// layout(input_attachment_index = GBufAtmtIdx_Depth, binding = Binding_GBufDepth) uniform subpassInput depthMap;

layout(location = VulkShaderLocation_TexCoord) in vec2 inTexCoord;

layout(location = 0) out vec4 outColor;

// note: assumes texCoord is already in normalized device coordinates
vec3 reconstructPosition(vec2 texCoord, float depth, mat4 invViewProj) {
	vec4 screenPos = vec4(texCoord * 2.0 - 1.0, depth * 2.0 - 1.0, 1.0);
	vec4 worldPos = invViewProj * screenPos;
	worldPos /= worldPos.w;
	return worldPos.xyz;
}

void main() {
    vec3 albedo = subpassLoad(albedoMap).rgb;
    float metallic = subpassLoad(materialMap).r;
	float roughness = subpassLoad(materialMap).g;
    float ao = subpassLoad(materialMap).b;
	vec2 hemioctNormal = subpassLoad(normalMap).xy; // VK_FORMAT_R16G16_SFLOAT stores normal in the xy channels
	vec3 N = hemioctToNormal(hemioctNormal);
	float depth = gl_FragCoord.z; // subpassLoad(depthMap).r; // VK_FORMAT_D32_SFLOAT stores depth in the red channel

	vec3 worldPos = reconstructPosition(inTexCoord, depth, invViewProj);

	vec3 color = vec3(0.0);
	for (int i = 0; i < VulkLights_NumLights; i++) {
		color += PBRForLight(lightsBuf.lights[i], eyePosUBO.eyePos, worldPos, albedo, metallic, roughness, N);
	}

	vec3 ambientLightColor = vec3(0.1); // TODO: get this from somewhere
	vec3 ambient = ao * albedo * ambientLightColor;
	outColor = vec4(color + ambient, 1.0);
}
