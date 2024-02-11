#version 450

#include "common.glsl"

XFORMS_UBO(xformUBO);
MODELXFORM_UBO(modelXformUBO);

layout(binding = VulkShaderBinding_NormalSampler) uniform sampler2D normSampler;

layout(binding = VulkShaderBinding_EyePos) uniform EyePos {
    vec3 eyePos;
} eyePosUBO;

layout(binding = VulkShaderBinding_Lights) uniform LightBuf {
    PointLight light;
} lightBuf;

layout(binding = VulkShaderBinding_MaterialUBO) uniform MaterialBuf {
    Material material;
} materialBuf;


FRAG_IN(inPosition, inNormal, inTangent, inTexCoord);
layout(location = 0) out vec4 outColor;

void main() {
    mat4 model = xformUBO.world * modelXformUBO.xform;
    vec3 norm = normalize(inNormal);
    vec3 tangent = normalize(inTangent);

    norm = vec3(texture(normSampler, inTexCoord));
    norm = norm * 2.0 - 1.0; // Remap from [0, 1] to [-1, 1]
    vec3 T = normalize(vec3(model * vec4(tangent, 0.0)));
    vec3 N = normalize(vec3(model * vec4(norm, 0.0)));
    vec3 B = cross(N, T);
    mat3 TBN = mat3(T, B, N);
    norm = normalize(TBN * norm);
    outColor = goochLighting(lightBuf.light, materialBuf.material, eyePosUBO.eyePos, norm, inPosition);
}