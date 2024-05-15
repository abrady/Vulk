#pragma once

struct VulkDebugNormalsUBO {
    float length = .1f;    // how long to render the debug normal
    bool useModel = false; // use the model's normals/tangents instead of the shader sampled normals
};

struct VulkDebugTangentsUBO {
    float length = .1f; // how long to render the debug tangent
};

struct VulkPBRDebugUBO {
    int32_t isMetallic = 0;
    float roughness = 0.5f;
    int32_t diffuse = true;
    int32_t specular = true;
};

struct VulkLightViewProjUBO {
    glm::mat4 viewProj;
};

struct VulkGlobalConstantsUBO {
    float viewportWidth;
    float viewportHeight;
};