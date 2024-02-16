#pragma once

enum VertBindingLocations {
    VertBindingLocations_PosBinding = 0,
    VertBindingLocations_NormalBinding = 1,
    VertBindingLocations_TangentBinding = 2,
    VertBindingLocations_TexCoordBinding = 3,
    VertBindingLocations_HeightBinding = 4,
    VertBindingLocations_Pos2Binding = 5,
    VertBindingLocations_MaxID = 6,
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
    VulkShaderBinding_MaxBindingID,
};

enum VulkShaderUBOBindings {
    VulkShaderUBOBinding_Xforms = VulkShaderBinding_XformsUBO,
    VulkShaderUBOBinding_Lights = VulkShaderBinding_Lights,
    VulkShaderUBOBinding_EyePos = VulkShaderBinding_EyePos,
    VulkShaderUBOBinding_WavesXform = VulkShaderBinding_WavesXform,
    VulkShaderUBOBinding_ModelXform = VulkShaderBinding_ModelXform,
    VulkShaderUBOBinding_MirrorPlaneUBO = VulkShaderBinding_MirrorPlaneUBO,
    VulkShaderUBOBinding_MaterialUBO = VulkShaderBinding_MaterialUBO,
    VulkShaderUBOBinding_DebugNormals = VulkShaderBinding_DebugNormalsUBO,
    VulkShaderUBOBinding_DebugTangents = VulkShaderBinding_DebugTangentsUBO,
    VulkShaderUBOBinding_MaxBindingID = VulkShaderUBOBinding_DebugTangents,
};

enum VulkShaderDebugUBOs {
    VulkShaderDebugUBO_DebugNormals = VulkShaderUBOBinding_DebugNormals,
    VulkShaderDebugUBO_DebugTangents = VulkShaderUBOBinding_DebugTangents,
    VulkShaderDebugUBO_MaxBindingID = VulkShaderDebugUBO_DebugTangents,
};

enum VulkShaderSSBOBindings {
    VulkShaderSSBOBinding_MaxBindingID = 0,
};

enum VulkShaderTextureBindings {
    VulkShaderTextureBinding_TextureSampler = VulkShaderBinding_TextureSampler,
    VulkShaderTextureBinding_TextureSampler2 = VulkShaderBinding_TextureSampler2,
    VulkShaderTextureBinding_TextureSampler3 = VulkShaderBinding_TextureSampler3,
    VulkShaderTextureBinding_NormalSampler = VulkShaderBinding_NormalSampler,
    VulkShaderTextureBinding_MaxBindingID = VulkShaderTextureBinding_NormalSampler,
};

struct VulkShaderEnums {
    static VertBindingLocations vertBindingFromString(const std::string &bindingName) {
        if (bindingName == "Pos") {
            return VertBindingLocations_PosBinding;
        } else if (bindingName == "Normal") {
            return VertBindingLocations_NormalBinding;
        } else if (bindingName == "Tangent") {
            return VertBindingLocations_TangentBinding;
        } else if (bindingName == "TexCoord") {
            return VertBindingLocations_TexCoordBinding;
        } else if (bindingName == "Height") {
            return VertBindingLocations_HeightBinding;
        } else if (bindingName == "Pos2") {
            return VertBindingLocations_Pos2Binding;
        } else {
            throw std::runtime_error("Unknown vertex binding: " + bindingName);
        }
        static_assert(VertBindingLocations_MaxID == 6);
    }
    static VulkShaderBindings shaderBindingFromString(const std::string &bindingName) {
        if (bindingName == "XformsUBO") {
            return VulkShaderBinding_XformsUBO;
        } else if (bindingName == "TextureSampler") {
            return VulkShaderBinding_TextureSampler;
        } else if (bindingName == "Lights") {
            return VulkShaderBinding_Lights;
        } else if (bindingName == "EyePos") {
            return VulkShaderBinding_EyePos;
        } else if (bindingName == "TextureSampler2") {
            return VulkShaderBinding_TextureSampler2;
        } else if (bindingName == "TextureSampler3") {
            return VulkShaderBinding_TextureSampler3;
        } else if (bindingName == "WavesXform") {
            return VulkShaderBinding_WavesXform;
        } else if (bindingName == "NormalSampler") {
            return VulkShaderBinding_NormalSampler;
        } else if (bindingName == "ModelXform") {
            return VulkShaderBinding_ModelXform;
        } else if (bindingName == "MirrorPlaneUBO") {
            return VulkShaderBinding_MirrorPlaneUBO;
        } else if (bindingName == "MaterialUBO") {
            return VulkShaderBinding_MaterialUBO;
        } else if (bindingName == "DebugNormalsUBO") {
            return VulkShaderBinding_DebugNormalsUBO;
        } else if (bindingName == "DebugTangentsUBO") {
            return VulkShaderBinding_DebugTangentsUBO;
        } else {
            throw std::runtime_error("Unknown shader binding: " + bindingName);
        }
        static_assert(VulkShaderBinding_MaxBindingID == 13);
    }

    static VulkShaderUBOBindings uboBindingFromString(const std::string &bindingName) {
        if (bindingName == "XformsUBO") {
            return VulkShaderUBOBinding_Xforms;
        } else if (bindingName == "Lights") {
            return VulkShaderUBOBinding_Lights;
        } else if (bindingName == "EyePos") {
            return VulkShaderUBOBinding_EyePos;
        } else if (bindingName == "WavesXform") {
            return VulkShaderUBOBinding_WavesXform;
        } else if (bindingName == "ModelXform") {
            return VulkShaderUBOBinding_ModelXform;
        } else if (bindingName == "MirrorPlaneUBO") {
            return VulkShaderUBOBinding_MirrorPlaneUBO;
        } else if (bindingName == "MaterialUBO") {
            return VulkShaderUBOBinding_MaterialUBO;
        } else if (bindingName == "DebugNormals") {
            return VulkShaderUBOBinding_DebugNormals;
        } else if (bindingName == "DebugTangents") {
            return VulkShaderUBOBinding_DebugTangents;
        } else {
            throw std::runtime_error("Unknown UBO binding: " + bindingName);
        }
        static_assert(VulkShaderUBOBinding_MaxBindingID == VulkShaderUBOBinding_DebugTangents);
    }

    static VulkShaderDebugUBOs debugUboBindingFromString(const std::string &bindingName) {
        if (bindingName == "DebugNormals") {
            return VulkShaderDebugUBO_DebugNormals;
        } else if (bindingName == "DebugTangents") {
            return VulkShaderDebugUBO_DebugTangents;
        } else {
            throw std::runtime_error("Unknown debug UBO binding: " + bindingName);
        }
        static_assert(VulkShaderDebugUBO_MaxBindingID == VulkShaderDebugUBO_DebugTangents);
    }

    static VulkShaderSSBOBindings ssboBindingFromString(const std::string &bindingName) {
        throw std::runtime_error("Unknown SSBO binding: " + bindingName);
    }

    static VulkShaderTextureBindings textureBindingFromString(const std::string &bindingName) {
        if (bindingName == "TextureSampler") {
            return VulkShaderTextureBinding_TextureSampler;
        } else if (bindingName == "TextureSampler2") {
            return VulkShaderTextureBinding_TextureSampler2;
        } else if (bindingName == "TextureSampler3") {
            return VulkShaderTextureBinding_TextureSampler3;
        } else if (bindingName == "NormalSampler") {
            return VulkShaderTextureBinding_NormalSampler;
        } else {
            throw std::runtime_error("Unknown texture binding: " + bindingName);
        }
        static_assert(VulkShaderTextureBinding_MaxBindingID == VulkShaderTextureBinding_NormalSampler);
    }
};