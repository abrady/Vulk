#version 450

#include "common.glsl"

layout(binding = VulkShaderBinding_XformsUBO) uniform UniformBufferObject {
    GlobalXform xform;
} xformUBO;

layout(binding = VulkShaderBinding_ModelXform) uniform ModelXformUBO {
    mat4 xform;
} modelUBO;

layout(location = LayoutLocation_Position) in vec3 inPosition;
layout(location = LayoutLocation_Normal) in vec3 inNormal;
// layout(location = LayoutLocation_Tangent) in vec3 inTangent;
layout(location = LayoutLocation_TexCoord) in vec2 inTexCoord;

layout(location = LayoutLocation_Position) out vec3 fragPos;  
layout(location = LayoutLocation_TexCoord) out vec2 fragTexCoord;

void main() {
    GlobalXform xform = xformUBO.xform;
    mat4 worldXform = xform.world * modelUBO.xform;
    gl_Position = xform.proj * xform.view * worldXform * vec4(inPosition, 1.0);
    fragTexCoord = inTexCoord;
    fragPos = vec3(worldXform * vec4(inPosition, 1.0));
}