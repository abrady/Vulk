#version 450

#include "common.glsl"

XFORMS_UBO(xform);
MODELXFORM_UBO(modelUBO);

layout(location = VulkShaderLocation_Pos) in vec3 inPosition;

void main() {
    gl_Position = xform.proj * xform.view * xform.world * modelUBO.xform * vec4(inPosition, 1.0);
}
