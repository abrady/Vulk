const float PI = 3.1415926535897932384626433832795;

const int VulkShaderBinding_XformsUBO = 0;
const int VulkShaderBinding_TextureSampler = 1;
const int VulkShaderBinding_Actors = 2;
const int VulkShaderBinding_Lights = 3;
const int VulkShaderBinding_Materials = 4;
const int VulkShaderBinding_EyePos = 5;
const int VulkShaderBinding_TextureSampler2 = 6;
const int VulkShaderBinding_TextureSampler3 = 7;
const int VulkShaderBinding_WavesXform = 8;
const int VulkShaderBinding_NormalSampler = 9;
const int VulkShaderBinding_ModelXform = 10;
const int VulkShaderBinding_MirrorPlaneUBO = 11;
const int VulkShaderBinding_MaterialUBO = 12;

struct GlobalXform {
    mat4 world;
    mat4 view;
    mat4 proj;
};

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

const int LayoutLocation_Position = 0;
const int LayoutLocation_Normal = 1; 
const int LayoutLocation_Tangent = 2;
const int LayoutLocation_TexCoord = 3;
const int LayoutLocation_Height = 4;
const int LayoutLocation_Bitangent = 5;

#define DECLARE_VERTEX_IN(inPosition, inNormal, inTangent, inTexCoord)  \
layout(location = LayoutLocation_Position) in vec3 inPosition; \
layout(location = LayoutLocation_Normal) in vec3 inNormal; \
layout(location = LayoutLocation_Tangent) in vec3 inTangent; \
layout(location = LayoutLocation_TexCoord) in vec2 inTexCoord

#define DECLARE_VERTEX_OUT(outPos, outNorm, outTangent, outBitangent) \
layout(location = LayoutLocation_Position) out vec3 outPos;  \
layout(location = LayoutLocation_Normal) out vec3 outNorm; \
layout(location = LayoutLocation_Tangent) out vec3 outTangent; \
layout(location = LayoutLocation_Bitangent) out vec3 outBitangent
