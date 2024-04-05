#version 450

#include "common.glsl"
#include "lighting.frag"

layout(binding = VulkShaderBinding_TextureSampler) uniform sampler2D texSampler;
layout(binding = VulkShaderBinding_NormalSampler) uniform sampler2D normSampler;
layout(binding = VulkShaderBinding_ShadowMapSampler) uniform sampler2D shadowSampler;

EYEPOS_UBO(eyePosUBO);
LIGHTS_UBO(lightBuf);

layout(location = VulkShaderLocation_Pos) in vec3 inPos;
layout(location = VulkShaderLocation_Normal) in vec3 inNormal;
layout(location = VulkShaderLocation_Tangent) in vec3 inTangent;
layout(location = VulkShaderLocation_TexCoord) in vec2 inTexCoord;
layout(location = VulkShaderLocation_PosLightSpace) in vec4 inPosLightSpace;

layout(location = 0) out vec4 outColor;

bool usePCF = false;

void main() {
    vec3 projCoords = inPosLightSpace.xyz / inPosLightSpace.w; // To NDC
    vec2 depthUVs = projCoords.xy * 0.5 + 0.5; // To texture coordinates
    float currentDepth = clamp(projCoords.z, 0.0, 1.0); // just in case the z is outside the frustum
    bool inShadow;
    float bias = 0.001;


    if (!usePCF) {
        float closestDepth = texture(shadowSampler, depthUVs).r;
        inShadow = currentDepth > closestDepth + bias;
    } else {
        // do a PCF to smooth out the shadow
        float shadow = 0.0;
        vec2 texelSize = 1.0 / textureSize(shadowSampler, 0);
        for (int x = -1; x <= 1; ++x) {
            for (int y = -1; y <= 1; ++y) {
                float pcfDepth = texture(shadowSampler, depthUVs + vec2(x, y) * texelSize).r;
                shadow += (currentDepth - bias > pcfDepth) ? 1.0 : 0.0;
            }
        }
        shadow /= 9.0;
        inShadow = shadow > 0.5;
    }

    // introducing a bias to reduce shadow acne. e.g. if we've sampled the same depth as we've projected
    // we'll get strange behavior so just expect it to be a biased amount further to be in shadow
    vec3 lightColor = lightBuf.light.color;
    vec4 tex = texture(texSampler, inTexCoord);
    vec3 norm = calcTBNNormal(normSampler, inTexCoord, inNormal, inTangent);
    if (inShadow) {
        // In shadow - handwave 
        tex *= 0.5;
        lightColor *= 0.01;
    } 
    outColor = blinnPhong(tex.xyx, norm, eyePosUBO.eyePos, lightBuf.light.pos, inPos, lightColor, true);
}