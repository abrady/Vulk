#version 450

#include "common.glsl"

XFORMS_UBO(xform);
MODELXFORM_UBO(modelUBO);
layout(binding = VulkShaderBinding_LightViewProjUBO) uniform LightData {
    mat4 viewProjMatrix;
} light;

layout(location = LayoutLocation_Position) in vec3 inPos;
layout(location = LayoutLocation_Position) out vec3 outPos;

void main()
{
    mat4 worldXform = xform.world * modelUBO.xform;
    vec4 pos = light.viewProjMatrix * worldXform * vec4(inPos, 1.0);
    outPos = pos.xyz/pos.w;
    gl_Position = pos;
}