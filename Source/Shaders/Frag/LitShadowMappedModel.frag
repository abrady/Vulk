#version 450

#include "common.glsl"
#include "lighting.frag"

layout(binding = VulkShaderBinding_TextureSampler) uniform sampler2D texSampler;
layout(binding = VulkShaderBinding_NormalSampler) uniform sampler2D normSampler;
layout(binding = VulkShaderBinding_ShadowSampler) uniform sampler2D shadowSampler;

EYEPOS_UBO(eyePosUBO);
LIGHTS_UBO(lightBuf);

layout(location = LayoutLocation_Position) in vec3 inPos;
layout(location = LayoutLocation_Normal) in vec3 inNormal;
layout(location = LayoutLocation_Tangent) in vec3 inTangent;
layout(location = LayoutLocation_TexCoord) in vec2 inTexCoord;
layout(location = LayoutLocation_PosLightSpace) in vec4 inPosLightSpace;

layout(location = 0) out vec4 outColor;

void main() {
    vec3 projCoords = inPosLightSpace.xyz / inPosLightSpace.w; // To NDC
    projCoords = projCoords * 0.5 + 0.5; // To texture coordinates
    float closestDepth = texture(shadowSampler, projCoords.xy).r;
    float currentDepth = clamp(projCoords.z, 0.0, 1.0); // just in case the z is outside the frustum
    // introducing a bias to reduce shadow acne. e.g. if we've sampled the same depth as we've projected
    // we'll get strange behavior so just expect it to be a biased amount further to be in shadow
    vec3 lightColor = lightBuf.light.color;
    vec4 tex = texture(texSampler, inTexCoord);
    vec3 norm = calcTBNNormal(normSampler, inTexCoord, inNormal, inTangent);
    if (currentDepth > closestDepth + 0.005) {
        // In shadow - handwave 
        tex *= 0.5;
        lightColor *= 0.01;
    } 
    outColor = blinnPhong(tex.xyx, norm, eyePosUBO.eyePos, lightBuf.light.pos, inPos, lightColor, true);
}