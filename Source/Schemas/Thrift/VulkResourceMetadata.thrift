namespace cpp vulk
    
include "VulkShaderEnums.thrift"

struct ShaderDef {
    1: string name;
    2: string path;
}

// Define enums
enum MeshDefType {
    Model,
    Mesh
}

enum GeoMeshDefType {
    Sphere,
    Cylinder,
    EquilateralTriangle,
    Quad,
    Grid,
    Axes
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
    FILL_RECTANGLE_NV = 1000153000 // NVidia extension
}

enum VulkCompareOp {
    NEVER = 0,
    LESS = 1,
    EQUAL = 2,
    LESS_OR_EQUAL = 3,
    GREATER = 4,
    NOT_EQUAL = 5,
    GREATER_OR_EQUAL = 6,
    ALWAYS = 7
}

enum VulkCullModeFlags {
    NONE = 0,
    FRONT = 1,
    BACK = 2,
    FRONT_AND_BACK = 3
}

enum VulkShaderStageFlagBits {
    Vertex = 0,
    Fragment,
    Geometry
}

// Define complex structures
struct DescriptorSetDef {
    // VkShaderStageFlagBits
    1: map<i32, list<VulkShaderEnums.VulkShaderUBOBinding>> uniformBuffers; 
    2: map<i32, list<VulkShaderEnums.VulkShaderSSBOBinding>> storageBuffers;
    3: map<i32, list<VulkShaderEnums.VulkShaderTextureBinding>> imageSamplers;
}
