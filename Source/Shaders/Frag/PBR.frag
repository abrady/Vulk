#version 450

#include "common.glsl"
#include "lighting.frag"

layout(binding = Binding_TextureSampler) uniform sampler2D albedoMap;
layout(binding = Binding_DisplacementSampler) uniform sampler2D displacementMap; // TODO: Implement displacement mapping
layout(binding = Binding_RoughnessSampler) uniform sampler2D roughnessMap;
layout(binding = Binding_AmbientOcclusionSampler) uniform sampler2D aoMap; //TODO: double check we're sampling this correctly
layout(binding = Binding_NormalSampler) uniform sampler2D normalMap;
layout(binding = Binding_MetallicSampler) uniform sampler2D metallicMap;

EYEPOS_UBO(eyePosUBO);

layout(binding = Binding_Lights) uniform LightBuf { 
    PointLight lights[VulkLights_NumLights]; 
} lightsBuf;

layout (std140, binding = Binding_PBRDebugUBO) uniform PBRDebug {
    uint isMetallic;      // 4 bytes
    float roughness;      // 4 bytes, follows directly because it's also 4-byte aligned
    bool diffuse;         // 4 bytes in GLSL
    bool specular;        // 4 bytes in GLSL
};

layout(location = VulkShaderLocation_Pos) in vec3 inPos;
layout(location = VulkShaderLocation_Normal) in vec3 inNormal;
layout(location = VulkShaderLocation_Tangent) in vec3 inTangent;
layout(location = VulkShaderLocation_Bitangent) in vec3 inBitangent;
layout(location = VulkShaderLocation_TexCoord) in vec2 inTexCoord;

layout(location = 0) out vec4 outColor;

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
    vec3 mapN = texture(normalMap, inTexCoord).rgb;
    vec3 N = sampleNormalMap(normalMap, inTexCoord, inNormal, inTangent, inBitangent);

    vec3 Lo = PBR(lightsBuf.lights, eyePosUBO.eyePos, inPos, N, albedo, metallic, roughness, ao);
    // Output final color
    vec4 color = vec4(Lo, 1.0);
    outColor = color;
}


