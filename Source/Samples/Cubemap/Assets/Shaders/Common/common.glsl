#ifndef COMMON_GLSL_H
#define COMMON_GLSL_H
#include "VulkShaderEnums_generated.glsl"

const float PI = 3.1415926535897932384626433832795;

struct Material
{
    vec3 Ka;  // Ambient color
    float Ns; // Specular exponent (shininess)
    vec3 Kd;  // Diffuse color
    float Ni; // Optical density (index of refraction)
    vec3 Ks;  // Specular color
    float d;  // Transparency (dissolve)
};

// duplicate HLSL's saturate
float saturate(float x) {
	return clamp(x, 0.0, 1.0);
}

// get the normal from the normal map and transform it into world space 
// (or whatever space the passed in tangent and normal are in)
// https://learnopengl.com/Advanced-Lighting/Normal-Mapping
mat3 calcTBNMat(vec3 normWorld, vec3 tangentWorld) {
    vec3 N = normalize(normWorld);
    vec3 T = normalize(tangentWorld);
    // re-orthogonalize T with respect to N
    T = normalize(T - dot(T, N) * N); // Gram-Schmidt orthogonalize
    vec3 B = cross(N, T);
    return mat3(T, B, N);
}

vec3 calcTBNNormal(sampler2D normSampler, vec2 inTexCoord, vec3 normWorld, vec3 tangentWorld) {
    vec3 norm = texture(normSampler, inTexCoord).xyz;
    norm = normalize(norm * 2.0 - 1.0); // Remap from [0, 1] to [-1, 1]
    mat3 TBN = calcTBNMat(normWorld, tangentWorld);
    norm = normalize(TBN * norm);
    return norm;
}

#define XFORMS_UBO(xformUBO)  \
layout(binding = Binding_XformsUBO) uniform UniformBufferObject { \
    mat4 world; \
    mat4 view; \
    mat4 proj; \
} xformUBO

#define MODELXFORM_UBO(modelUBO)  \
layout(binding = Binding_ModelXform) uniform ModelXformUBO { \
    mat4 xform; \
} modelUBO

#define EYEPOS_UBO(eyePosUBO)  \
layout(binding = Binding_EyePos) uniform EyePos { \
    vec3 eyePos; \
} eyePosUBO

#define LIGHTS_UBO(lightUBO)  \
layout(binding = Binding_Lights) uniform LightBuf { \
    PointLight light; \
} lightBuf

#define DEBUGNORMALS_UBO(debugNormalsUBO)  \
layout(binding = Binding_DebugNormalsUBO) uniform DebugNormalsUBO { \
    float len;         \
    bool useModel;     \
} debugNormalsUBO

#define MATERIAL_UBO(materialUBO)  \
layout(binding = Binding_MaterialUBO) uniform MaterialBuf { \
    Material material; \
} materialBuf



#define VERTEX_IN(inPosition, inNormal, inTangent, inTexCoord)  \
layout(location = VulkShaderLocation_Pos) in vec3 inPosition; \
layout(location = VulkShaderLocation_Normal) in vec3 inNormal; \
layout(location = VulkShaderLocation_Tangent) in vec3 inTangent; \
layout(location = VulkShaderLocation_TexCoord) in vec2 inTexCoord



#endif // COMMON_GLSL_H