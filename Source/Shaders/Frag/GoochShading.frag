#version 450

#include "lighting.frag"
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


layout(location = LayoutLocation_Position) in vec3 inPosition;
layout(location = LayoutLocation_Normal) in vec3 inNormal;
layout(location = LayoutLocation_Tangent) in vec3 inTangent;
layout(location = LayoutLocation_TexCoord) in vec2 inTexCoord;

layout(location = 0) out vec4 outColor;

// c_shaded = s*c_highlight + (1-s)*t*c_warm + (1 - t)*c_cool
// where:
// c_cool = (0,0,0.55) * .25 * c_diffuse
// c_warm = (0.3,0.3,0) * .25 * c_diffuse
// c_highlight = (1,1,1)
// t = 0.5 * (1 + n.l)
// r = 2(n.l)n - l : e.g. reflect(n,l)
// s = clamp(100(r.v) - 87) : e.g. in the range 0 to 1
vec4 goochLighting(PointLight light, Material mtl, vec3 eyePos, vec3 fragNormal, vec3 fragPos) { 
    vec3 l = normalize(light.pos - fragPos);
    vec3 n = normalize(fragNormal);
    vec3 v = normalize(eyePos - fragPos);
    vec3 c_cool = vec3(0,0,0.55) * .25 * mtl.Kd;
    vec3 c_warm = vec3(0.3,0.3,0) * .25 * mtl.Kd;
    vec3 c_highlight = vec3(1,1,1);
    float t = 0.5 * (1 + dot(n,l));
    vec3 r = reflect(-l, n);
    float s = clamp(100 * dot(r,v) - 87, 0, 1);
    vec3 c_shaded = s * c_highlight + (1-s) * t * c_warm + (1 - t) * c_cool;
    return vec4(c_shaded, 1.0);
}

void main() {
    vec3 norm = calcTBNNormal(normSampler, inTexCoord, inNormal, inTangent);
    outColor = goochLighting(lightBuf.light, materialBuf.material, eyePosUBO.eyePos, norm, inPosition);
}