#version 450

#include "common.glsl"
#include "lighting.frag"

layout(binding = VulkShaderBinding_TextureSampler) uniform sampler2D texSampler;
layout(binding = VulkShaderBinding_NormalSampler) uniform sampler2D normSampler;

EYEPOS_UBO(eyePosUBO);
LIGHTS_UBO(lightBuf);

layout(location = VulkShaderLocation_Pos) in vec3 fragPos;  
layout(location = VulkShaderLocation_TexCoord) in vec2 fragTexCoord;
layout(location = VulkShaderLocation_Normal) in vec3 fragNormal;
layout(location = VulkShaderLocation_Tangent) in vec3 fragTangent; // not used, but part of standard params.

layout(location = 0) out vec4 outColor;

void main() {
    vec4 tex = texture(texSampler, fragTexCoord);
    outColor = blinnPhong(tex.xyx, fragNormal, eyePosUBO.eyePos, lightBuf.light.pos, fragPos, lightBuf.light.color, true);
}