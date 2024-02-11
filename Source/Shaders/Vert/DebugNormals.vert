#version 450
/**
* This vertex shader will extrac the normals from a texture and pass them to the geometry shader to generate
*/

#include "common.glsl"

MODELXFORM_UBO(modelUBO);

layout(binding = VulkShaderBinding_NormalSampler) uniform sampler2D normSampler;

VERTEX_IN(inPosition, inNormal, inTangent, inTexCoord);
VERTEX_OUT(outPos, outNorm, outTangent, outTexCoord);

void main() {
    vec4 pos = modelUBO.xform * vec4(inPosition, 1.0);
    outPos = pos.xyz;
    gl_Position = modelUBO.xform * pos;

    vec3 norm = inNormal;
    //vec3 norm = texture(normSampler, inTexCoord).xyz;
    outNorm = (modelUBO.xform * vec4(norm, 0.0)).xyz;

    vec3 tangent = inTangent;
    outTangent = (modelUBO.xform * vec4(tangent, 0.0)).xyz;

    vec3 bitangent = cross(norm, tangent);
    outTexCoord = inTexCoord;
}