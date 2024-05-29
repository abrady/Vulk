#version 450

#include "common.glsl"

XFORMS_UBO(xform);
MODELXFORM_UBO(modelUBO);
layout(binding = VulkShaderBinding_LightViewProjUBO) uniform LightData {
    mat4 viewProjMatrix;
} light;

layout(location = VulkShaderLocation_Pos) in vec3 inPos;
// layout(location = VulkShaderLocation_Pos) out vec3 outPos;
// layout(location = VulkShaderLocation_PosLightSpace) out vec4 outLightSpacePos;

void main()
{
    mat4 worldXform = xform.world * modelUBO.xform;
    vec4 pos = light.viewProjMatrix * worldXform * vec4(inPos, 1.0);
    // outLightSpacePos = pos;
    // outPos = pos.xyz/pos.w;
    gl_Position = pos;
}