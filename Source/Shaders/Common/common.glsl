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


// note that the order matters here: it allows this to be packed into 2 vec4s
struct PointLight {
    vec3 pos;           // point light only
    float falloffStart; // point/spot light only
    vec3 color;         // color of light
    float falloffEnd;   // point/spot light only    
};

struct Material
{
    vec3 Ka;  // Ambient color
    float Ns; // Specular exponent (shininess)
    vec3 Kd;  // Diffuse color
    float Ni; // Optical density (index of refraction)
    vec3 Ks;  // Specular color
    float d;  // Transparency (dissolve)
};

/**
* @brief Calculates the lighting for a single light source using the Phong lighting model.
* @param diffuseIn: an additional diffuse color to be added to the material's diffuse color. typically from a texture.
*/
vec4 basicLighting(PointLight light, Material mtl, vec4 diffuseIn, vec3 eyePos, vec3 fragNormal, vec3 fragPos) {
    fragNormal = normalize(fragNormal);
    vec4 lightColor = vec4(light.color, 1.0);
    vec3 lightDir = normalize(light.pos - fragPos);
    float lambert = max(dot(fragNormal, lightDir), 0.0);
    vec3 normEyeVec = normalize(eyePos - fragPos);

    vec4 matDiffuse = vec4(mtl.Kd, mtl.d) * diffuseIn;

    // A_l⊗m_d
    vec4 ambient = vec4(mtl.Ka, mtl.d) * matDiffuse;

    // B*max(N . L, 0)⊗m_d
    vec4 diffuse = lambert * lightColor * matDiffuse;

    // max(L.n,0)B⊗[F_0 + (1 - F_0)(1 - N.v)^5][(m + 8)/8(n.h)^m]
    // vec4 R0 = vec4(material.fresnelR0, 1.0);
    vec4 R0 = vec4(mtl.Ks, mtl.d);
    vec3 h = normalize(normEyeVec + lightDir);
    vec4 fresnel = R0 + (vec4(1.0) - R0) * pow(1.0 - dot(fragNormal,normEyeVec), 5.0);
    float m = (100 - mtl.Ns) * .1; // HACK: just faking it for now, Ns is usually 0-100 but may be up to 1000
    float nDotH = max(dot(fragNormal, h), 0);
    float D = (m + 2.0) / (2.0 * PI) * pow(nDotH, m);
    vec4 microfacet = vec4(D);
    vec4 specular = lightColor * lambert * fresnel * microfacet;

    // combine
    vec4 acc = ambient;
    acc += diffuse;
    acc += specular;
    return acc;
    // return (ambient + diffuse + specular);
}

// get the normal from the normal map and transform it into world space 
// (or whatever space the passed in tangent and normal are in)
// https://learnopengl.com/Advanced-Lighting/Normal-Mapping
vec3 calcTBNNormal(sampler2D normSampler, vec2 inTexCoord, vec3 normWorld, vec3 tangentWorld) {
    vec3 norm = vec3(texture(normSampler, inTexCoord));
    norm = norm * 2.0 - 1.0; // Remap from [0, 1] to [-1, 1]
    vec3 N = normalize(normWorld);
    vec3 T = normalize(tangentWorld);
    vec3 B = cross(N, T);
    mat3 TBN = mat3(T, B, N);
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