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
    vec3 diffuse = diff * texColor * lightColor;
    // specular
    vec3 viewDir = normalize(viewPos - fragPos);
    float spec = 0.0;
    if (blinn) {
        vec3 halfwayDir = normalize(lightDir + viewDir);  
        spec = pow(max(dot(normal, halfwayDir), 0.0), 32.0);
    } else {
        vec3 reflectDir = reflect(-lightDir, normal);
        spec = pow(max(dot(viewDir, reflectDir), 0.0), 8.0);
    }
    vec3 specular = spec * lightColor;
    return vec4(ambient + diffuse + specular, 1.0);
}

// The DistributionGGX, aka Trowbridge-Reitz distribution, function is part of the Cook-Torrance BRDF and is used to define the microfacet distribution of a surface. 
// The GGX distribution is popular for its plausible appearance and computational efficiency for rough surfaces. 
// This function returns the probability density of the microfacets oriented along the half-vector H. 
// The GGX function is used because it provides a good approximation of surfaces with varying roughness and works well in a variety of lighting conditions.
float DistributionGGX(vec3 N, vec3 H, float roughness) {
    float a = roughness * roughness;
    float a2 = a * a;
    float NdotH = max(dot(N, H), 0.0);
    float NdotH2 = NdotH * NdotH;

    float num = a2;
    float denom = (NdotH2 * (a2 - 1.0) + 1.0);
    denom = PI * denom * denom;

    return num / denom;
}

// Schlick's approximation for the Smith's shadowing function
// Schlick's approximation is a formula used to approximate the Fresnel reflectance, which is part of the larger rendering equation. 
// In the context of the Smith shadowing function in microfacet models, it provides a way to approximate the geometric shadowing term 
// which accounts for the microfacets that block each other (self-shadowing) due to their orientation.
float GeometrySchlickGGX(float NdotV, float roughness) {
    float a = roughness;
    float k = (a * a) / 2.0;

    float nom = NdotV;
    float denom = NdotV * (1.0 - k) + k;

    return nom / denom;
}

// Smith's shadowing function combines two Schlick-GGX terms for the light and view direction
float GeometrySmith(vec3 N, vec3 V, vec3 L, float roughness) {
    float NdotV = max(dot(N, V), 0.0);
    float NdotL = max(dot(N, L), 0.0);
    float ggx1 = GeometrySchlickGGX(NdotV, roughness);
    float ggx2 = GeometrySchlickGGX(NdotL, roughness);

    return ggx1 * ggx2;
}

vec3 FresnelSchlick(float cosTheta, vec3 F0) {
    return F0 + (1.0 - F0) * pow(1.0 - cosTheta, 5.0);
}

vec3 PBRDirectIllumination(vec3 albedo, int i, vec3 N, vec3 V, vec3 L, float roughness, float metalness, vec3 F0, float ao) {
    vec3 H = normalize(V + L);
    float distance = length(lightsBuf.lights[i].pos - inPos);
    float attenuation = 1.0 / (distance * distance);
    vec3 radiance = lightsBuf.lights[i].color * attenuation;

    // Cook-Torrance BRDF
    float NDF = DistributionGGX(N, H, roughness);
    float G = GeometrySmith(N, V, L, roughness);
    vec3 F = FresnelSchlick(max(dot(H, V), 0.0), F0);

    vec3 kS = F;
    vec3 kD = vec3(1.0) - kS;
    kD *= 1.0 - metalness;

    vec3 numerator = NDF * G * F;
    float denominator = 4.0 * max(dot(N, V), 0.0) * max(dot(N, L), 0.0) + 0.001; // Avoid divide by zero
    vec3 specular = numerator / denominator;

    // Combine results
    vec3 ambient = albedo * ao;
    vec3 color = ambient + (kD * albedo / PI + specular) * radiance;
    return color;
}


// Note: assumes all params incoming have had no transformations applied
vec3 PBR(PointLight lights[VulkLights_NumLights], vec3 inPos, vec3 inNormal, vec3 inTangent, vec3 inBitangent, vec3 mapN, vec3 albedo, float metallic, float roughness, float ao) {
    vec3 N = normalize(inNormal);
    mapN = mapN * 2.0 - 1.0;
    vec3 T = normalize(inTangent);
    vec3 B = normalize(inBitangent);
    N = normalize(T * mapN.x + B * mapN.y + N * mapN.z);

    // Calculate reflectance at normal incidence (F0)
    // pick something between our base F0 and the albedo color
    // this is a fairly standard value for dielectrics (non-metals) and will be used for most materials
    // while metals use the albedo color which will be much more energetic.
    // Some common F0 values for diaelectrics are:
    // Water 0.02
    // Skin 0.028  
    // Hair 0.046
    // Fabric 0.04–0.056 
    // Stone 0.035–0.056
    // Plastics/glass 0.04–0.05
    // Gems 0.05–0.08
    // Diamond-like 0.13–0.2
    // The realtime rendering book says 0.04 is a good value for most dielectrics
    // 
    // and some common F0 values for metals are:
    // Titanium 0.542,0.497,0.449
    // Chromium 0.549,0.556,0.554
    // Iron 0.562,0.565,0.578
    // Copper 0.955,0.638,0.538
    // metals are almost always .5 or above and vary across the spectrum (hence the triples)
    //
    // NOTE: this assumes the surface is in air, if the surface is in a different medium the F0 value will change
    // # Given F0 in air
    // F0_air = 0.13
    // # Calculate the index of refraction of the material using the air F0 value
    // n_material = (1 + math.sqrt(F0_air)) / (1 - math.sqrt(F0_air))
    // # Index of refraction of water
    // n_water = 1.33
    // # Calculate the F0 in water
    // F0_water = ((n_material - n_water) / (n_material + n_water)) ** 2

    vec3 F0 = vec3(0.04);
    F0 = mix(F0, albedo, metallic);

    // Calculate lighting
    vec3 V = normalize(eyePosUBO.eyePos - inPos);
    vec3 Lo = vec3(0.0);
    for(int i = 0; i < VulkLights_NumLights; i++) {
        vec3 L = normalize(lightsBuf.lights[i].pos - inPos);
        Lo += PBRDirectIllumination(albedo, i, N, V, L, roughness, metallic, F0, ao) * lightsBuf.lights[i].color;
    }
}
