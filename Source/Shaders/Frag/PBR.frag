#version 450

#include "common.glsl"
#include "lighting.frag"

layout(binding = VulkShaderBinding_TextureSampler) uniform sampler2D albedoMap;
layout(binding = VulkShaderBinding_NormalSampler) uniform sampler2D normalMap;
layout(binding = VulkShaderBinding_AmbientOcclusionSampler) uniform sampler2D aoMap;
layout(binding = VulkShaderBinding_DisplacementSampler) uniform sampler2D displacementMap;
layout(binding = VulkShaderBinding_RoughnessSampler) uniform sampler2D roughnessMap;
layout(binding = VulkShaderBinding_MetallicSampler) uniform sampler2D metallicMap;

EYEPOS_UBO(eyePosUBO);

layout(binding = VulkShaderBinding_Lights) uniform LightBuf { 
    PointLight lights[VulkLights_NumLights]; 
} lightsBuf;


layout(location = VulkShaderLocation_Pos) in vec3 inPos;
layout(location = VulkShaderLocation_Normal) in vec3 inNormal;
layout(location = VulkShaderLocation_Tangent) in vec3 inTangent;
layout(location = VulkShaderLocation_Bitangent) in vec3 inBitangent;
layout(location = VulkShaderLocation_TexCoord) in vec2 inTexCoord;

layout(location = 0) out vec4 outColor;

// Function prototypes
vec3 GetDirectIllumination(int i, vec3 N, vec3 V, vec3 L, float roughness, float metalness, vec3 F0, float ao);
vec3 FresnelSchlick(float cosTheta, vec3 F0);
float DistributionGGX(vec3 N, vec3 H, float roughness);
float GeometrySchlickGGX(float NdotV, float roughness);
float GeometrySmith(vec3 N, vec3 V, vec3 L, float roughness);

// void main() {
//     vec4 tex = texture(texSampler, inTexCoord);
//     vec3 norm = calcTBNNormal(normSampler, inTexCoord, inNormal, inTangent);
//     //outColor = basicLighting(lightBuf.light, materialBuf.material, tex, eyePosUBO.eyePos, norm, inPos);
//     outColor = blinnPhong(tex.xyx, norm, eyePosUBO.eyePos, lightBuf.light.pos, inPos, lightBuf.light.color, true);
// }

// Main fragment shader function
void main() {
    // Sample textures
    vec3 albedo = texture(albedoMap, inTexCoord).rgb;
    float metallic = texture(metallicMap, inTexCoord).r;
    float roughness = texture(roughnessMap, inTexCoord).r;
    float ao = texture(aoMap, inTexCoord).r;

    // Apply normal map
    vec3 N = normalize(inNormal);
    vec3 mapN = texture(normalMap, inTexCoord).rgb;
    mapN = mapN * 2.0 - 1.0;
    vec3 T = normalize(inTangent);
    vec3 B = normalize(inBitangent);
    N = normalize(T * mapN.x + B * mapN.y + N * mapN.z);

    // Calculate reflectance at normal incidence (F0)
    vec3 F0 = vec3(0.04);
    F0 = mix(F0, albedo, metallic);

    // Calculate lighting
    vec3 V = normalize(eyePosUBO.eyePos - inPos);
    vec3 Lo = vec3(0.0);
    for(int i = 0; i < VulkLights_NumLights; i++) {
        vec3 L = normalize(lightsBuf.lights[i].pos - inPos);
        Lo += GetDirectIllumination(i, N, V, L, roughness, metallic, F0, ao) * lightsBuf.lights[i].color;
    }

    // Output final color
    vec4 color = vec4(Lo, 1.0);
    outColor = color;
}

// Lighting calculation functions
vec3 GetDirectIllumination(int i, vec3 N, vec3 V, vec3 L, float roughness, float metalness, vec3 F0, float ao) {
    vec3 H = normalize(V + L);
    float distance = length(lightsBuf.lights[i].pos - inPos);
    float attenuation = 1.0 / (distance * distance);
    vec3 radiance = lightsBuf.lights[i].color * attenuation;

    // Cook-Torrance BRDF
    float NDF = DistributionGGX(N, H, roughness);
    float G = GeometrySmith(N, V, L, roughness);
    vec3 F = FresnelSchlick(max(dot(H, V), 0.0), F0);

    vec3 kS = F;
    vec3 kD = vec3(1.0) - kS;
    kD *= 1.0 - metalness;

    vec3 numerator = NDF * G * F;
    float denominator = 4.0 * max(dot(N, V), 0.0) * max(dot(N, L), 0.0) + 0.001; // Avoid divide by zero
    vec3 specular = numerator / denominator;

    // Combine results
    vec3 ambient = albedo * ao;
    vec3 color = ambient + (kD * albedo / PI + specular) * radiance;
    return color;
}

// The DistributionGGX function is part of the Cook-Torrance BRDF and is used to define the microfacet distribution of a surface. 
// The GGX distribution is popular for its plausible appearance and computational efficiency for rough surfaces. 
// This function returns the probability density of the microfacets oriented along the half-vector H. 
// The GGX function is used because it provides a good approximation of surfaces with varying roughness and works well in a variety of lighting conditions.
float DistributionGGX(vec3 N, vec3 H, float roughness) {
    float a = roughness * roughness;
    float a2 = a * a;
    float NdotH = max(dot(N, H), 0.0);
    float NdotH2 = NdotH * NdotH;

    float num = a2;
    float denom = (NdotH2 * (a2 - 1.0) + 1.0);
    denom = PI * denom * denom;

    return num / denom;
}

// Schlick's approximation for the Smith's shadowing function
// Schlick's approximation is a formula used to approximate the Fresnel reflectance, which is part of the larger rendering equation. 
// In the context of the Smith shadowing function in microfacet models, it provides a way to approximate the geometric shadowing term 
// which accounts for the microfacets that block each other (self-shadowing) due to their orientation.
float GeometrySchlickGGX(float NdotV, float roughness) {
    float a = roughness;
    float k = (a * a) / 2.0;

    float nom = NdotV;
    float denom = NdotV * (1.0 - k) + k;

    return nom / denom;
}

// Smith's shadowing function combines two Schlick-GGX terms for the light and view direction
float GeometrySmith(vec3 N, vec3 V, vec3 L, float roughness) {
    float NdotV = max(dot(N, V), 0.0);
    float NdotL = max(dot(N, L), 0.0);
    float ggx1 = GeometrySchlickGGX(NdotV, roughness);
    float ggx2 = GeometrySchlickGGX(NdotL, roughness);

    return ggx1 * ggx2;
}

vec3 FresnelSchlick(float cosTheta, vec3 F0) {
    return F0 + (1.0 - F0) * pow(1.0 - cosTheta, 5.0);
}


