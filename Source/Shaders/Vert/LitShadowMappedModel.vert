#version 450


#extension GL_GOOGLE_include_directive : require
#include "common.glsl"

XFORMS_UBO(xform);
MODELXFORM_UBO(modelUBO);
layout(binding = VulkShaderBinding_LightViewProjUBO) uniform LightData {
    mat4 viewProjMatrix;
} light;

VERTEX_IN(inPosition, inNormal, inTangent, inTexCoord);

layout(location = LayoutLocation_Position) out vec3 outPos; 
layout(location = LayoutLocation_Normal) out vec3 outNorm;
layout(location = LayoutLocation_Tangent) out vec3 outTangent;
layout(location = LayoutLocation_TexCoord) out vec2 outTexCoord;
layout(location = LayoutLocation_PosLightSpace) out vec4 outPosLightSpace;

void main() {
    mat4 worldXform = xform.world * modelUBO.xform;
    gl_Position = xform.proj * xform.view * worldXform * vec4(inPosition, 1.0);
    outTexCoord = inTexCoord;
    outPos = vec3(worldXform * vec4(inPosition, 1.0));
    outNorm = vec3(worldXform * vec4(inNormal, 0.0));
    outTangent = vec3(worldXform * vec4(inTangent, 0.0));
    outPosLightSpace = light.viewProjMatrix * vec4(outPos, 1.0);
}