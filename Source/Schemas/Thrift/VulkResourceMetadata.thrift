namespace cpp vulk
    
include "VulkShaderEnums.thrift"

// Define enums
enum MeshDefType {
    Model = 0,
    Mesh = 1,
}

enum GeoMeshDefType {
    Sphere = 1,
    Cylinder = 2,
    EquilateralTriangle = 3,
    Quad = 4,
    Grid = 5,
    Axes = 6
}

enum VulkCullModeFlags {
    NONE = 0,
    FRONT = 1,
    BACK = 2,
    FRONT_AND_BACK = 3
}

enum VulkShaderStageFlagBits {
    Vertex = 0,
    Fragment = 1,
    Geometry = 2,
}


struct PushConstantDef {
    1: i32 stageFlags; // or-ed together
    2: i32 size;
}

// Define complex structures
struct DescriptorSetDef {
    1: string name;
    // VkShaderStageFlagBits
    2: map<i32, list<VulkShaderEnums.VulkShaderUBOBinding>> uniformBuffers; 
    3: map<i32, list<VulkShaderEnums.VulkShaderSSBOBinding>> storageBuffers;
    4: map<i32, list<VulkShaderEnums.VulkShaderTextureBinding>> imageSamplers;
}

struct PipelineBlendingDef {
    1: bool enabled;
    2: VulkShaderEnums.VulkBlendFactor srcColorBlendFactor;
    3: VulkShaderEnums.VulkBlendFactor dstColorBlendFactor;
    4: VulkShaderEnums.VulkBlendOp colorBlendOp;
    5: VulkShaderEnums.VulkBlendFactor srcAlphaBlendFactor;
    6: VulkShaderEnums.VulkBlendFactor dstAlphaBlendFactor;
    7: VulkShaderEnums.VulkBlendOp alphaBlendOp;
    8: string colorWriteMask; // some set of RBGA
}

struct BuiltPipelineDef {
    1: i32 version;
    2: string name;
    3: string vertShaderName;
    4: string geomShaderName;
    5: string fragShaderName;
    6: DescriptorSetDef descriptorSetDef;
    7: list<VulkShaderEnums.VulkShaderLocation> vertInputs;
    8: list<PushConstantDef> pushConstants;
    9: VulkShaderEnums.VulkPrimitiveTopology primitiveTopology;
    10: VulkShaderEnums.VulkPolygonMode polygonMode;
    11: bool depthTestEnabled;
    12: bool depthWriteEnabled;
    13: VulkShaderEnums.VulkCompareOp depthCompareOp;
    14: i32 cullMode; // VkCullModeFlags
    15: PipelineBlendingDef blending;
}

struct Vec3 {
    1: double x;
    2: double y;
    3: double z;
}

struct Xform {
    1: Vec3 pos;
    2: Vec3 rot;
    3: Vec3 scale;
}

struct GeoSphereDef {
    1: double radius;
    2: i32 numSubdivisions;
}

struct GeoCylinderDef {
    1: double height;
    2: double bottomRadius;
    3: double topRadius;
    4: i32 numStacks;
    5: i32 numSlices;
}

struct GeoEquilateralTriangleDef {
    1: double sideLength;
    2: i32 numSubdivisions;
}

struct GeoQuadDef {
    1: double w;
    2: double h;
    3: i32 numSubdivisions;
}

struct GeoGridDef {
    1: double width;
    2: double depth;
    3: i32 m;
    4: i32 n;
    5: double repeatU;
    6: double repeatV;
}

struct GeoAxesDef {
    1: double length;
}

union GeoMeshDef {
    1: GeoSphereDef sphere;
    2: GeoCylinderDef cylinder;
    3: GeoEquilateralTriangleDef triangle;
    4: GeoQuadDef quad;
    5: GeoGridDef grid;
    6: GeoAxesDef axes;
}


struct ModelDef {
    1: string name;
    2: string meshName;
    3: string materialName;
    4: MeshDefType meshDefType;
    5: GeoMeshDefType geoMeshDefType;
    6: GeoMeshDef geoMeshDef;
}

struct ActorDef {
    1: string name;
    2: string pipelineName;
    3: string modelName;
    4: ModelDef inlineModel; // mutually exclusive with modelName
    5: Xform xform;
}

struct CameraDef {
    1: Vec3 eye;
    2: Vec3 lookAt;
    3: double nearClip;
    4: double farClip;
}

struct LightDef {
    1: VulkShaderEnums.LightType type;
    2: Vec3 pos;
    3: Vec3 color;
    4: double falloffStart;
    5: double falloffEnd;
}

struct SceneDef {
    1: string name;
    2: CameraDef camera;
    3: list<ActorDef> actors;
    4: list<LightDef> lights;
}

struct ShaderDef {
    1: string name;
    2: string path;
}
