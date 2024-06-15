#version 450

#include "common.glsl"

// layout(push_constant) uniform pickPushConstants {
//     uint objectID;
// } pc;

// layout(location = VulkShaderLocation_Pos) in vec3 inPos;
layout(location = 0) out vec4 outColor;
void main()
{
    outColor = vec4(1,1,0,1);
}
