#version 450

#include "common.glsl"
#include "lighting.frag"

layout(binding = VulkShaderBinding_EyePos) uniform EyePos { 
    vec3 eyePos; 
} eyePosUBO;

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
	vec2 iResolution; // unused...
} globalConstants;

layout(binding = VulkShaderBinding_TextureSampler) uniform sampler2D albedoMap;
layout(binding = VulkShaderBinding_NormalSampler) uniform sampler2D normalMap;
layout(binding = VulkShaderBinding_DisplacementSampler) uniform sampler2D displacementMap; // TODO: Implement displacement mapping
layout(binding = VulkShaderBinding_RoughnessSampler) uniform sampler2D roughnessMap;
layout(binding = VulkShaderBinding_AmbientOcclusionSampler) uniform sampler2D aoMap; //TODO: double check we're sampling this correctly
layout(binding = VulkShaderBinding_MetallicSampler) uniform sampler2D metallicMap;
// layout(binding = VulkShaderBinding_ShadowMapSampler) uniform sampler2D shadowSampler;
// layout(binding = VulkShaderBinding_CubemapSampler) uniform samplerCube cubemapSampler;

layout(location = VulkShaderLocation_Pos) in vec3 inPos; // TODO: not needed
layout(location = VulkShaderLocation_Normal) in vec3 inNormal;
layout(location = VulkShaderLocation_Tangent) in vec3 inTangent;
layout(location = VulkShaderLocation_Bitangent) in vec3 inBitangent;
layout(location = VulkShaderLocation_TexCoord) in vec2 inTexCoord;

layout(location = VulkGBufAttachment_Albedo) out vec4 outAlbedo;
layout(location = VulkGBufAttachment_Normal) out vec2 outNormal;
layout(location = VulkGBufAttachment_Depth) out float outDepth;
layout(location = VulkGBufAttachment_Material) out vec4 outMaterial;

void main() {
    // Sample the textures
    vec3 albedo = texture(albedoMap, inTexCoord).rgb;
	vec3 normal = sampleNormalMap(normalMap, inTexCoord, inNormal, inTangent, inBitangent);
    float metallic = texture(metallicMap, inTexCoord).r;
    float roughness = texture(roughnessMap, inTexCoord).r;
    float ao = texture(aoMap, inTexCoord).r;

    // Write to the G-Buffers
	outAlbedo = vec4(albedo, 1.0);
	outNormal = normalToHemioct(normal);
	outDepth = gl_FragCoord.z;
	outMaterial = vec4(metallic, roughness, ao, 0.0);
}