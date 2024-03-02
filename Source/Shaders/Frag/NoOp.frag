#version 450

#include "common.glsl"


layout(location = LayoutLocation_Position) in vec3 inPos;
layout(location = LayoutLocation_Normal) in vec3 inNorm;
layout(location = LayoutLocation_Tangent) in vec3 inTangent;
layout(location = LayoutLocation_TexCoord) in vec2 inTexCoord;

// this shader is used to generate a depth buffer for the shadow map.
// and potentially other things.
void main()
{
    // don't do nothing.
}
