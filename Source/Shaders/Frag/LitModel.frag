#version 450

#include "common.glsl"
#include "lighting.frag"

layout(binding = VulkShaderBinding_TextureSampler) uniform sampler2D texSampler;
layout(binding = VulkShaderBinding_NormalSampler) uniform sampler2D normSampler;

EYEPOS_UBO(eyePosUBO);
LIGHTS_UBO(lightBuf);

layout(location = VulkShaderLocation_Pos) in vec3 inPos;
layout(location = VulkShaderLocation_Normal) in vec3 inNormal;
layout(location = VulkShaderLocation_Tangent) in vec3 inTangent;
layout(location = VulkShaderLocation_TexCoord) in vec2 inTexCoord;

layout(location = 0) out vec4 outColor;

void main() {
    vec4 tex = texture(texSampler, inTexCoord);
    vec3 norm = calcTBNNormal(normSampler, inTexCoord, inNormal, inTangent);
    //outColor = basicLighting(lightBuf.light, materialBuf.material, tex, eyePosUBO.eyePos, norm, inPos);
    outColor = blinnPhong(tex.xyx, norm, eyePosUBO.eyePos, lightBuf.light.pos, inPos, lightBuf.light.color, true);
}