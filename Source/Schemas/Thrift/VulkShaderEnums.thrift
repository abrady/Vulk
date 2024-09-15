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
    CubemapCoord = 9
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
    GlobalConstantsUBO = 20,
    CubemapSampler = 21,
    GBufNormal = 22,
    GBufDepth = 23,
    GBufAlbedo = 24,
    GBufMaterial = 25,
    InvViewProjUBO = 27,
}

// ================================================
// GBuffer enums
// - bindings: index bound to in a shader
// - attachment indices: index of the attachment in the framebuffer
// - input attachment indices: index of the input attachment in the shader
//
// (this is a bit of a mess)
// ================================================
enum GBufBinding {
    Normal = 22,
    Depth = 23,
    Albedo = 24,
    Material = 25,
}


// attachments are bound to framebuffers and typically written to by the fragment shader
enum GBufAtmtIdx {
    Color = 0, // the index of the swapchain image
    Albedo = 1,
    Normal = 2,
    Material = 3,
    Depth = 4,
}

// ugh this is a mess
enum GBufInputAtmtIdx {
    Albedo = 0,
    Normal = 1,
    Material = 2,
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
    GlobalConstantsUBO = 20,
    InvViewProjUBO = 27,
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
    RoughnessSampler = 18,
    CubemapSampler = 21,
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

enum VulkPrimitiveTopology {
    PointList = 0,
    LineList = 1,
    LineStrip = 2,
    TriangleList = 3,
    TriangleStrip = 4,
    TriangleFan = 5,
    LineListWithAdjacency = 6,
    LineStripWithAdjacency = 7,
    TriangleListWithAdjacency = 8,
    TriangleStripWithAdjacency = 9,
    PatchList = 10
}

enum VulkPolygonMode {
    FILL = 0,
    LINE = 1,
    POINT = 2,
    FILL_RECTANGLE_NV = 1000153000
}

enum VulkCompareOp {
    NEVER = 0,
    LESS = 1,
    EQUAL = 2,
    LESS_OR_EQUAL = 3,
    GREATER = 4,
    NOT_EQUAL = 5,
    GREATER_OR_EQUAL = 6,
    ALWAYS = 7,
}

enum VulkBlendFactor {
    ZERO = 0,
    ONE = 1,
    SRC_COLOR = 2,
    ONE_MINUS_SRC_COLOR = 3,
    DST_COLOR = 4,
    ONE_MINUS_DST_COLOR = 5,
    SRC_ALPHA = 6,
    ONE_MINUS_SRC_ALPHA = 7,
    DST_ALPHA = 8,
    ONE_MINUS_DST_ALPHA = 9,
    CONSTANT_COLOR = 10,
    ONE_MINUS_CONSTANT_COLOR = 11,
    CONSTANT_ALPHA = 12,
    ONE_MINUS_CONSTANT_ALPHA = 13,
    SRC_ALPHA_SATURATE = 14,
    SRC1_COLOR = 15,
    ONE_MINUS_SRC1_COLOR = 16,
    SRC1_ALPHA = 17,
    ONE_MINUS_SRC1_ALPHA = 18,
}

enum VulkBlendOp {
    ADD = 0,
    SUBTRACT = 1,
    REVERSE_SUBTRACT = 2,
    MIN = 3,
    MAX = 4,
    ZERO_EXT = 1000148000,
    SRC_EXT = 1000148001,
    DST_EXT = 1000148002,
    SRC_OVER_EXT = 1000148003,
    DST_OVER_EXT = 1000148004,
    SRC_IN_EXT = 1000148005,
    DST_IN_EXT = 1000148006,
    SRC_OUT_EXT = 1000148007,
    DST_OUT_EXT = 1000148008,
    SRC_ATOP_EXT = 1000148009,
    DST_ATOP_EXT = 1000148010,
    XOR_EXT = 1000148011,
    MULTIPLY_EXT = 1000148012,
    SCREEN_EXT = 1000148013,
    OVERLAY_EXT = 1000148014,
    DARKEN_EXT = 1000148015,
    LIGHTEN_EXT = 1000148016,
    COLORDODGE_EXT = 1000148017,
    COLORBURN_EXT = 1000148018,
    HARDLIGHT_EXT = 1000148019,
    SOFTLIGHT_EXT = 1000148020,
    DIFFERENCE_EXT = 1000148021,
    EXCLUSION_EXT = 1000148022,
    INVERT_EXT = 1000148023,
    INVERT_RGB_EXT = 1000148024,
    LINEARDODGE_EXT = 1000148025,
    LINEARBURN_EXT = 1000148026,
    VIVIDLIGHT_EXT = 1000148027,
    LINEARLIGHT_EXT = 1000148028,
    PINLIGHT_EXT = 1000148029,
    HARDMIX_EXT = 1000148030,
    HSL_HUE_EXT = 1000148031,
    HSL_SATURATION_EXT = 1000148032,
    HSL_COLOR_EXT = 1000148033,
    HSL_LUMINOSITY_EXT = 1000148034,
    PLUS_EXT = 1000148035,
    PLUS_CLAMPED_EXT = 1000148036,
    PLUS_CLAMPED_ALPHA_EXT = 1000148037,
    PLUS_DARKER_EXT = 1000148038,
    MINUS_EXT = 1000148039,
    MINUS_CLAMPED_EXT = 1000148040,
    CONTRAST_EXT = 1000148041,
    INVERT_OVG_EXT = 1000148042,
    RED_EXT = 1000148043,
    GREEN_EXT = 1000148044,
    BLUE_EXT = 1000148045,
}

enum LightType {
    Point = 0,
    Directional = 1,
    Spot = 2
}
