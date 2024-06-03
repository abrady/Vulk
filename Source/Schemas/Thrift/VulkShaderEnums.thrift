// Define enums
namespace cpp vulk

enum VulkShaderLocation {
    Color = 0,
    Pos = 1,
    Normal = 2,
    Tangent = 3,
    TexCoord = 4,
    Height = 5,
    Pos2 = 6,
    PosLightSpace = 7,
    Bitangent = 8
}

enum VulkShaderBinding {
    XformsUBO = 0,
    TextureSampler = 1,
    Lights = 2,
    EyePos = 3,
    TextureSampler2 = 4,
    TextureSampler3 = 5,
    WavesXform = 6,
    NormalSampler = 7,
    ModelXform = 8,
    MirrorPlaneUBO = 9,
    MaterialUBO = 10,
    DebugNormalsUBO = 11,
    DebugTangentsUBO = 12,
    LightViewProjUBO = 13,
    ShadowMapSampler = 14,
    AmbientOcclusionSampler = 15,
    DisplacementSampler = 16,
    MetallicSampler = 17,
    RoughnessSampler = 18,
    PBRDebugUBO = 19,
    GlobalConstantsUBO = 20
    MAX = 21
}

enum VulkShaderUBOBinding {
    Xforms = 0,
    Lights = 2,
    EyePos = 3,
    ModelXform = 8,
    MaterialUBO = 10,
    DebugNormals = 11,
    DebugTangents = 12,
    LightViewProjUBO = 13,
    PBRDebugUBO = 19,
    GlobalConstantsUBO = 20
    MAX = 20
}

enum VulkShaderDebugUBO {
    DebugNormals = 11,
    DebugTangents = 12,
    PBRDebugUBO = 19
}

enum VulkShaderSSBOBinding {
    MaxBindingID = 0
}

enum VulkShaderTextureBinding {
    TextureSampler = 1,
    TextureSampler2 = 4,
    TextureSampler3 = 5,
    NormalSampler = 7,
    ShadowMapSampler = 14,
    AmbientOcclusionSampler = 15,
    DisplacementSampler = 16,
    MetallicSampler = 17,
    RoughnessSampler = 18
    MAX = 18
}

enum VulkLights {
    NumLights = 4
}

enum VulkShaderStage {
    VERTEX = 0x00000001,
    TESSELLATION_CONTROL = 0x00000002,
    TESSELLATION_EVALUATION = 0x00000004,
    GEOMETRY = 0x00000008,
    FRAGMENT = 0x00000010,
    COMPUTE = 0x00000020
}