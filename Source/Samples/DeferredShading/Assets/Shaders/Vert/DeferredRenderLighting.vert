#version 450

#include "common.glsl"

XFORMS_UBO(xform);
MODELXFORM_UBO(modelUBO);

VERTEX_IN(inPosition, inNormal, inTangent, inTexCoord);

layout(location = VulkShaderLocation_Pos) out vec3 outPos; 
layout(location = VulkShaderLocation_Normal) out vec3 outNorm;
layout(location = VulkShaderLocation_Tangent) out vec3 outTangent;
layout(location = VulkShaderLocation_Bitangent) out vec3 outBitangent;
layout(location = VulkShaderLocation_TexCoord) out vec2 outTexCoord;
layout(location = VulkShaderLocation_CubemapCoord) out vec3 outCubemapCoord;

void main() {
    mat4 worldXform = xform.world * modelUBO.xform;
    vec4 worldPos = worldXform * vec4(inPosition, 1.0);
    outCubemapCoord = normalize(worldPos.xyz);
    gl_Position = xform.proj * xform.view * worldPos;
    outTexCoord = inTexCoord;
    outPos = vec3(worldXform * vec4(inPosition, 1.0));
    outNorm = vec3(worldXform * vec4(inNormal, 0.0));
    outTangent = vec3(worldXform * vec4(inTangent, 0.0));
    outBitangent = cross(outNorm, outTangent);
}
