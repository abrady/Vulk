const float PI = 3.1415926535897932384626433832795;

const int VulkShaderBinding_XformsUBO = 0;
const int VulkShaderBinding_TextureSampler = 1;
const int VulkShaderBinding_Lights = 2;
const int VulkShaderBinding_EyePos = 3;
const int VulkShaderBinding_TextureSampler2 = 4;
const int VulkShaderBinding_TextureSampler3 = 5;
const int VulkShaderBinding_WavesXform = 6;
const int VulkShaderBinding_NormalSampler = 7;
const int VulkShaderBinding_ModelXform = 8;
const int VulkShaderBinding_MirrorPlaneUBO = 9;
const int VulkShaderBinding_MaterialUBO = 10;
const int VulkShaderBinding_DebugNormalsUBO = 11;
const int VulkShaderBinding_DebugTangentsUBO = 12;



struct Material
{
    vec3 Ka;  // Ambient color
    float Ns; // Specular exponent (shininess)
    vec3 Kd;  // Diffuse color
    float Ni; // Optical density (index of refraction)
    vec3 Ks;  // Specular color
    float d;  // Transparency (dissolve)
};

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

const int LayoutLocation_Position = 0;
const int LayoutLocation_Normal = 1; 
const int LayoutLocation_Tangent = 2;
const int LayoutLocation_TexCoord = 3;
const int LayoutLocation_Height = 4;
const int LayoutLocation_Position2 = 5;

#define XFORMS_UBO(xformUBO)  \
layout(binding = VulkShaderBinding_XformsUBO) uniform UniformBufferObject { \
    mat4 world; \
    mat4 view; \
    mat4 proj; \
} xformUBO

#define MODELXFORM_UBO(modelUBO)  \
layout(binding = VulkShaderBinding_ModelXform) uniform ModelXformUBO { \
    mat4 xform; \
} modelUBO


#define VERTEX_IN(inPosition, inNormal, inTangent, inTexCoord)  \
layout(location = LayoutLocation_Position) in vec3 inPosition; \
layout(location = LayoutLocation_Normal) in vec3 inNormal; \
layout(location = LayoutLocation_Tangent) in vec3 inTangent; \
layout(location = LayoutLocation_TexCoord) in vec2 inTexCoord

#define VERTEX_OUT(outPos, outNorm, outTangent, outTexCoord) \
layout(location = LayoutLocation_Position) out vec3 outPos;  \
layout(location = LayoutLocation_Normal) out vec3 outNorm; \
layout(location = LayoutLocation_Tangent) out vec3 outTangent; \
layout(location = LayoutLocation_TexCoord) out vec2 outTexCoord

#define FRAG_IN(inPosition, inNormal, inTangent, inTexCoord)  \
layout(location = LayoutLocation_Position) in vec3 inPosition; \
layout(location = LayoutLocation_Normal) in vec3 inNormal; \
layout(location = LayoutLocation_Tangent) in vec3 inTangent; \
layout(location = LayoutLocation_TexCoord) in vec2 inTexCoord