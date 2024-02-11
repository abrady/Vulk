#version 450

#include "common.glsl"

DECLARE_XFORMS_UBO(xform);
DECLARE_MODELXFORM_UBO(modelUBO);

DECLARE_VERTEX_IN(inPosition, inNormal, inTangent, inTexCoord);
DECLARE_VERTEX_OUT(outPos, outNorm, outTangent, outTexCoord);

void main() {
    mat4 worldXform = xform.world * modelUBO.xform;
    gl_Position = xform.proj * xform.view * worldXform * vec4(inPosition, 1.0);
    outTexCoord = inTexCoord;
    outPos = vec3(worldXform * vec4(inPosition, 1.0));
    outNorm = vec3(worldXform * vec4(inNormal, 0.0));
    outTangent = vec3(worldXform * vec4(inTangent, 0.0)); 
}