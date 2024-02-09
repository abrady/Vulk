#version 450

#include "common.glsl"

layout(binding = VulkShaderBinding_NormalSampler) uniform sampler2D normSampler;

layout(binding = VulkShaderBinding_EyePos) uniform EyePos {
    vec3 eyePos;
} eyePosUBO;

layout(binding = VulkShaderBinding_Lights) uniform LightBuf {
    PointLight light;
} lightBuf;

layout(binding = VulkShaderBinding_MaterialUBO) uniform MaterialBuf {
    Material material;
} materialBuf;


layout(location = LayoutLocation_Position) in vec3 fragPos;  
layout(location = LayoutLocation_TexCoord) in vec2 fragTexCoord;

layout(location = 0) out vec4 outColor;

void main() {
    vec3 norm = vec3(texture(normSampler, fragTexCoord));
    outColor = goochLighting(lightBuf.light, materialBuf.material, eyePosUBO.eyePos, norm, fragPos);
}