// note that the order matters here: it allows this to be packed into 2 vec4s
struct PointLight {
    vec3 pos;           // point light only
    float falloffStart; // point/spot light only
    vec3 color;         // color of light
    float falloffEnd;   // point/spot light only    
};

vec4 blinnPhong(vec3 texColor, vec3 normal, vec3 viewPos, vec3 lightPos, vec3 fragPos, vec3 lightColor, bool blinn) {
    // ambient
    vec3 ambient = 0.05 * texColor;
    // diffuse
    vec3 lightDir = normalize(lightPos - fragPos);
    float diff = max(dot(lightDir, normal), 0.0);
    vec3 diffuse = diff * texColor;
    // specular
    vec3 viewDir = normalize(viewPos - fragPos);
    vec3 reflectDir = reflect(-lightDir, normal);
    float spec = 0.0;
    if(blinn)
    {
        vec3 halfwayDir = normalize(lightDir + viewDir);  
        spec = pow(max(dot(normal, halfwayDir), 0.0), 32.0);
    }
    else
    {
        vec3 reflectDir = reflect(-lightDir, normal);
        spec = pow(max(dot(viewDir, reflectDir), 0.0), 8.0);
    }
    vec3 specular = vec3(0.3) * spec; // assuming bright white light texColor
    return vec4(ambient + diffuse + specular, 1.0);
}

/**
* @brief Calculates the lighting for a single light source using the Phong lighting model.
* @param diffuseIn: an additional diffuse color to be added to the material's diffuse color. typically from a texture.
*/
// vec4 basicLighting(PointLight light, Material mtl, vec4 diffuseIn, vec3 eyePos, vec3 fragNormal, vec3 fragPos) {
//     fragNormal = normalize(fragNormal);
//     vec4 lightColor = vec4(light.color, 1.0);
//     vec3 lightDir = normalize(light.pos - fragPos);
//     float lambert = max(dot(fragNormal, lightDir), 0.0);
//     vec3 normEyeVec = normalize(eyePos - fragPos);

//     vec4 matDiffuse = vec4(mtl.Kd, mtl.d) * diffuseIn;

//     // A_l⊗m_d
//     vec4 ambient = vec4(mtl.Ka, mtl.d) * matDiffuse;

//     // B*max(N . L, 0)⊗m_d
//     vec4 diffuse = lambert * lightColor * matDiffuse;

//     // max(L.n,0)B⊗[F_0 + (1 - F_0)(1 - N.v)^5][(m + 8)/8(n.h)^m]
//     // vec4 R0 = vec4(material.fresnelR0, 1.0);
//     vec4 R0 = vec4(mtl.Ks, mtl.d);
//     vec3 h = normalize(normEyeVec + lightDir);
//     vec4 fresnel = R0 + (vec4(1.0) - R0) * pow(1.0 - dot(fragNormal,normEyeVec), 5.0);
//     float m = (100 - mtl.Ns) * .1; // HACK: just faking it for now, Ns is usually 0-100 but may be up to 1000
//     float nDotH = max(dot(fragNormal, h), 0);
//     float D = (m + 2.0) / (2.0 * PI) * pow(nDotH, m);
//     vec4 microfacet = vec4(D);
//     vec4 specular = lightColor * lambert * fresnel * microfacet;

//     // combine
//     vec4 acc = ambient;
//     acc += diffuse;
//     acc += specular;
//     return acc;
//     // return (ambient + diffuse + specular);
// }
