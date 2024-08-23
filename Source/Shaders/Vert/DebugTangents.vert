#version 450
/**
* This vertex shader will extrac the normals from a texture and pass them to the geometry shader to generate
*/

#include "common.glsl"

XFORMS_UBO(xformUBO);
MODELXFORM_UBO(modelUBO);

layout(binding = Binding_DebugTangentsUBO) uniform DebugTangentsUBO {
    float len;        
} debugTangentsUBO;

layout(binding = Binding_NormalSampler) uniform sampler2D normSampler;

VERTEX_IN(inPosition, inNormal, inTangent, inTexCoord);

layout(location = VulkShaderLocation_Pos) out vec3 outWorldPos;
layout(location = VulkShaderLocation_Normal) out vec3 outWorldNorm;
layout(location = VulkShaderLocation_Pos2) out vec4 outProjPos; // the offset for the normal

void main() {
    mat4 worldXform = xformUBO.world * modelUBO.xform;    
    vec4 pos = worldXform * vec4(inPosition, 1.0);
    gl_Position = xformUBO.proj * xformUBO.view * pos;
    outWorldPos = pos.xyz;

    vec4 outPos2;
    outWorldNorm = (worldXform * vec4(inTangent, 0.0)).xyz;
    outPos2 = vec4(outWorldPos + outWorldNorm * debugTangentsUBO.len, 1);
    outProjPos = xformUBO.proj * xformUBO.view * outPos2;
}