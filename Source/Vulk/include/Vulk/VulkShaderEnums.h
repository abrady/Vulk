#pragma once

#include <stdexcept>
#include <string>

enum VulkVertBindingLocation {
    VulkVertBindingLocation_ColorBinding = 0,
    VulkVertBindingLocation_PosBinding = 1,
    VulkVertBindingLocation_NormalBinding = 2,
    VulkVertBindingLocation_TangentBinding = 3,
    VulkVertBindingLocation_TexCoordBinding = 4,
    VulkVertBindingLocation_HeightBinding = 5,
    VulkVertBindingLocation_Pos2Binding = 6,
    VulkVertBindingLocation_PosLightSpace = 7,
    VulkVertBindingLocation_MaxID = 8,
};

// keep in sync with Source\Shaders\Common\common.glsl
// every binding needs to be globally unique across all shaders in a given pipeline
enum VulkShaderBindings {
    VulkShaderBinding_XformsUBO = 0,
    VulkShaderBinding_TextureSampler = 1,
    VulkShaderBinding_Lights = 2,
    VulkShaderBinding_EyePos = 3,
    VulkShaderBinding_TextureSampler2 = 4,
    VulkShaderBinding_TextureSampler3 = 5,
    VulkShaderBinding_WavesXform = 6,
    VulkShaderBinding_NormalSampler = 7,
    VulkShaderBinding_ModelXform = 8,
    VulkShaderBinding_MirrorPlaneUBO = 9,
    VulkShaderBinding_MaterialUBO = 10,
    VulkShaderBinding_DebugNormalsUBO = 11,
    VulkShaderBinding_DebugTangentsUBO = 12,
    VulkShaderBinding_LightViewProjUBO = 13,
    VulkShaderBinding_ShadowSampler = 14,
    VulkShaderBinding_MaxBindingID,
};

enum VulkShaderUBOBinding {
    VulkShaderUBOBinding_Xforms = VulkShaderBinding_XformsUBO,
    VulkShaderUBOBinding_Lights = VulkShaderBinding_Lights,
    VulkShaderUBOBinding_EyePos = VulkShaderBinding_EyePos,
    VulkShaderUBOBinding_ModelXform = VulkShaderBinding_ModelXform,
    VulkShaderUBOBinding_MaterialUBO = VulkShaderBinding_MaterialUBO,
    VulkShaderUBOBinding_DebugNormals = VulkShaderBinding_DebugNormalsUBO,
    VulkShaderUBOBinding_DebugTangents = VulkShaderBinding_DebugTangentsUBO,
    VulkShaderUBOBinding_LightViewProjUBO = VulkShaderBinding_LightViewProjUBO,
    VulkShaderUBOBinding_MAX = VulkShaderUBOBinding_LightViewProjUBO,
};

enum VulkShaderDebugUBOs {
    VulkShaderDebugUBO_DebugNormals = VulkShaderUBOBinding_DebugNormals,
    VulkShaderDebugUBO_DebugTangents = VulkShaderUBOBinding_DebugTangents,
    VulkShaderDebugUBO_MaxBindingID = VulkShaderDebugUBO_DebugTangents,
};

enum VulkShaderSSBOBinding {
    VulkShaderSSBOBinding_MaxBindingID = 0,
};

enum VulkShaderTextureBinding {
    VulkShaderTextureBinding_TextureSampler = VulkShaderBinding_TextureSampler,
    VulkShaderTextureBinding_TextureSampler2 = VulkShaderBinding_TextureSampler2,
    VulkShaderTextureBinding_TextureSampler3 = VulkShaderBinding_TextureSampler3,
    VulkShaderTextureBinding_NormalSampler = VulkShaderBinding_NormalSampler,
    VulkShaderTextureBinding_ShadowSampler = VulkShaderBinding_ShadowSampler,
    VulkShaderTextureBinding_MAX = VulkShaderTextureBinding_ShadowSampler,
};
