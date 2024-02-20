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
    VulkVertBindingLocation_MaxID = 7,
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
    VulkShaderUBOBinding_ModelXform = VulkShaderBinding_ModelXform,
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
    static VulkVertBindingLocation vertBindingFromString(const std::string &bindingName) {
        if (bindingName == "Color") {
            return VulkVertBindingLocation_ColorBinding;
        } else if (bindingName == "Pos") {
            return VulkVertBindingLocation_PosBinding;
        } else if (bindingName == "Normal") {
            return VulkVertBindingLocation_NormalBinding;
        } else if (bindingName == "Tangent") {
            return VulkVertBindingLocation_TangentBinding;
        } else if (bindingName == "TexCoord") {
            return VulkVertBindingLocation_TexCoordBinding;
        } else if (bindingName == "Height") {
            return VulkVertBindingLocation_HeightBinding;
        } else if (bindingName == "Pos2") {
            return VulkVertBindingLocation_Pos2Binding;
        } else {
            throw std::runtime_error("Unknown vertex binding: " + bindingName);
        }
        static_assert(VulkVertBindingLocation_MaxID == 7);
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
        } else if (bindingName == "ModelXform") {
            return VulkShaderUBOBinding_ModelXform;
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

    static std::string vertBindingLocationToString(VulkVertBindingLocation binding) {
        switch (binding) {
        case VulkVertBindingLocation_ColorBinding:
            return "LayoutLocation_Color";
        case VulkVertBindingLocation_PosBinding:
            return "LayoutLocation_Position";
        case VulkVertBindingLocation_NormalBinding:
            return "LayoutLocation_Normal";
        case VulkVertBindingLocation_TangentBinding:
            return "LayoutLocation_Tangent";
        case VulkVertBindingLocation_TexCoordBinding:
            return "LayoutLocation_TexCoord";
        case VulkVertBindingLocation_HeightBinding:
            return "LayoutLocation_Height";
        case VulkVertBindingLocation_Pos2Binding:
            return "LayoutLocation_Position2";
        default:
            throw std::runtime_error("Unknown vertex binding: " + std::to_string(binding));
        }
    }

    static std::string shaderBindingToString(uint32_t binding) {
        switch (binding) {
        case VulkShaderBinding_XformsUBO:
            return "VulkShaderBinding_XformsUBO";
        case VulkShaderBinding_TextureSampler:
            return "VulkShaderBinding_TextureSampler";
        case VulkShaderBinding_Lights:
            return "VulkShaderBinding_Lights";
        case VulkShaderBinding_EyePos:
            return "VulkShaderBinding_EyePos";
        case VulkShaderBinding_TextureSampler2:
            return "VulkShaderBinding_TextureSampler2";
        case VulkShaderBinding_TextureSampler3:
            return "VulkShaderBinding_TextureSampler3";
        case VulkShaderBinding_WavesXform:
            return "VulkShaderBinding_WavesXform";
        case VulkShaderBinding_NormalSampler:
            return "NormalSampler";
        case VulkShaderBinding_ModelXform:
            return "VulkShaderBinding_ModelXform";
        case VulkShaderBinding_MirrorPlaneUBO:
            return "VulkShaderBinding_MirrorPlaneUBO";
        case VulkShaderBinding_MaterialUBO:
            return "VulkShaderBinding_MaterialUBO";
        case VulkShaderBinding_DebugNormalsUBO:
            return "VulkShaderBinding_DebugNormalsUBO";
        case VulkShaderBinding_DebugTangentsUBO:
            return "VulkShaderBinding_DebugTangentsUBO";
        default:
            throw std::runtime_error("Unknown shader binding: " + std::to_string(binding));
        }
    }
};
