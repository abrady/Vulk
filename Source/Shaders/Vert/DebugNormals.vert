#version 450
/**
* This vertex shader will extrac the normals from a texture and pass them to the geometry shader to generate
*/
#include "common.glsl"

XFORMS_UBO(xformUBO);
MODELXFORM_UBO(modelUBO);

layout(binding = VulkShaderBinding_DebugNormalsUBO) uniform DebugNormalsUBO {
    float len;        
    bool useModel;       
} debugNormalsUBO;

layout(binding = VulkShaderBinding_NormalSampler) uniform sampler2D normSampler;

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
    // normal from texture
    if (!debugNormalsUBO.useModel) {
        vec3 normWorld = (worldXform * vec4(inNormal, 0.0)).xyz;
        vec3 tangentWorld = (worldXform * vec4(inTangent, 0.0)).xyz;
        outWorldNorm = calcTBNNormal(normSampler, inTexCoord, normWorld, tangentWorld);
        outPos2 = vec4(outWorldPos + outWorldNorm * 0.1, 1);
    } else {
        outWorldNorm = (worldXform * vec4(inNormal, 0.0)).xyz;
        outPos2 = vec4(outWorldPos + outWorldNorm * 0.1, 1);
    }

    outProjPos = xformUBO.proj * xformUBO.view * outPos2;
}