#version 450

#include "common.glsl"
#include "lighting.frag"

layout(binding = VulkShaderBinding_TextureSampler) uniform sampler2D albedoMap;
layout(binding = VulkShaderBinding_DisplacementSampler) uniform sampler2D displacementMap; // TODO: Implement displacement mapping
layout(binding = VulkShaderBinding_RoughnessSampler) uniform sampler2D roughnessMap;
layout(binding = VulkShaderBinding_AmbientOcclusionSampler) uniform sampler2D aoMap; //TODO: double check we're sampling this correctly
layout(binding = VulkShaderBinding_NormalSampler) uniform sampler2D normalMap;
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

    vec3 Lo = PBR(lightsBuf.lights, eyePosUBO.eyePos, inPos, inNormal, inTangent, inBitangent, mapN, albedo, metallic, roughness, ao);
    // Output final color
    vec4 color = vec4(Lo, 1.0);
    outColor = color;
}


