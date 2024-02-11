#version 450
/**
* This vertex shader will extrac the normals from a texture and pass them to the geometry shader to generate
*/

#include "common.glsl"

XFORMS_UBO(xformUBO);
MODELXFORM_UBO(modelUBO);

layout(binding = VulkShaderBinding_NormalSampler) uniform sampler2D normSampler;

VERTEX_IN(inPosition, inNormal, inTangent, inTexCoord);

layout(location = LayoutLocation_Position) out vec3 outWorldPos;
layout(location = LayoutLocation_Normal) out vec3 outWorldNorm;
layout(location = LayoutLocation_Position2) out vec4 outProjPos; // the offset for the normal

void main() {
    mat4 worldXform = xformUBO.world * modelUBO.xform;    
    vec4 pos = worldXform * vec4(inPosition, 1.0);
    gl_Position = xformUBO.proj * xformUBO.view * pos;
    outWorldPos = pos.xyz;

    vec3 normWorld = (worldXform * vec4(inNormal, 0.0)).xyz;
    vec3 tangentWorld = (worldXform * vec4(inTangent, 0.0)).xyz;
    outWorldNorm = calcTBNNormal(normSampler, inTexCoord, normWorld, tangentWorld);

    outProjPos = xformUBO.proj * xformUBO.view * vec4(outWorldPos + outWorldNorm * 0.1, 1);
}