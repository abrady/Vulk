#version 450
/**
* This vertex shader will extrac the normals from a texture and pass them to the geometry shader to generate
*/
#include "common.glsl"

XFORMS_UBO(xformUBO);
// MODELXFORM_UBO(modelUBO);

// layout(push_constant) uniform pickPushConstants {
//     uint objectID;
// } pc;

layout(location = VulkShaderLocation_Pos) in vec3 inPosition;

// layout(location = VulkShaderLocation_Pos) out vec3 outWorldPos;
// layout(location = VulkShaderLocation_Normal) out vec3 outWorldNorm;
// layout(location = VulkShaderLocation_Pos2) out vec4 outProjPos; // the offset for the normal

void main() {
    // outWorldPos = (xformUBO.world * vec4(inPosition, 1.0)).xyz;
    gl_Position = xformUBO.proj * xformUBO.view * xformUBO.world * vec4(inPosition, 1.0);
}