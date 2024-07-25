namespace cpp vulk
    
include "VulkShaderEnums.thrift"

// Define enums
enum MeshDefType {
    Model = 0,
    Mesh = 1,
}

enum VulkCullModeFlags {
    NONE = 0,
    FRONT = 1,
    BACK = 2,
    FRONT_AND_BACK = 3
}

struct PushConstantDef {
    1: i32 stageFlags; // or-ed together
    2: i32 size;
}

// Define complex structures
struct DescriptorSetDef {
    // the keys are VkShaderStageFlagBits
    2: map<i32, list<VulkShaderEnums.VulkShaderUBOBinding>> uniformBuffers; 
    3: map<i32, list<VulkShaderEnums.VulkShaderSSBOBinding>> storageBuffers;
    4: map<i32, list<VulkShaderEnums.VulkShaderTextureBinding>> imageSamplers;
}

struct PipelineBlendingDef {
    1: bool enabled = false;
    2: VulkShaderEnums.VulkBlendFactor srcColorBlendFactor = VulkShaderEnums.VulkBlendFactor.ONE;
    3: VulkShaderEnums.VulkBlendFactor dstColorBlendFactor = VulkShaderEnums.VulkBlendFactor.ZERO;
    4: VulkShaderEnums.VulkBlendOp colorBlendOp = VulkShaderEnums.VulkBlendOp.ADD;
    5: VulkShaderEnums.VulkBlendFactor srcAlphaBlendFactor = VulkShaderEnums.VulkBlendFactor.ONE;
    6: VulkShaderEnums.VulkBlendFactor dstAlphaBlendFactor = VulkShaderEnums.VulkBlendFactor.ZERO;
    7: VulkShaderEnums.VulkBlendOp alphaBlendOp = VulkShaderEnums.VulkBlendOp.ADD;
    8: string colorWriteMask = "RGBA"; // some set of RBGA
}

struct SrcPipelineDef {
    1: i32 version;
    2: string name;
    3: string vertShader;
    4: string geomShader;
    5: string fragShader;
    6: string primitiveTopology; // VulkShaderEnums.VulkPrimitiveTopology
    7: bool depthTestEnabled;
    8: bool depthWriteEnabled;
    9: string depthCompareOp; // VulkShaderEnums.VulkCompareOp
    10: string polygonMode; // VulkShaderEnums.VulkPolygonMode
    11: string cullMode; // VulkShaderEnums.VulkCullModeFlag/VkCullModeFlags
    12: list<PipelineBlendingDef> colorBlends;  
    13: i32 subpass;
}

struct PipelineDef {
    1: i32 version;
    2: string name;
    3: string vertShader;
    4: string geomShader;
    5: string fragShader;
    6: DescriptorSetDef descriptorSetDef;
    7: list<VulkShaderEnums.VulkShaderLocation> vertInputs;
    8: list<PushConstantDef> pushConstants;
    9: VulkShaderEnums.VulkPrimitiveTopology primitiveTopology = VulkShaderEnums.VulkPrimitiveTopology.TriangleList;
    10: VulkShaderEnums.VulkPolygonMode polygonMode = VulkShaderEnums.VulkPolygonMode.FILL;
    11: bool depthTestEnabled = true;
    12: bool depthWriteEnabled = true;
    13: VulkShaderEnums.VulkCompareOp depthCompareOp = VulkShaderEnums.VulkCompareOp.LESS;
    14: VulkCullModeFlags cullMode = VulkCullModeFlags.BACK; // VkCullModeFlags
    15: list<PipelineBlendingDef> colorBlends;  
    16: i32 subpass;
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


struct XformDef {
    1: Vec3 pos;
    2: Vec3 rot;
    3: Vec3 scale;
}

struct ModelDef {
    1: string name;
    2: string mesh;
    3: string material;
    4: MeshDefType meshDefType;
    6: GeoMeshDef geoMesh;
    7: XformDef xform;
}

struct ActorDef {
    1: string name;
    2: string pipeline;
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
    6: string name;
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

struct ResourceConfig {
    1: string ResourcesDir;
}

struct SrcProjectDef {
    1: string name;
    2: string startingScene;
    3: list<string> sceneNames;
}

struct ProjectDef {
    1: string name;
    2: map<string, SceneDef> scenes;
    3: map<string, PipelineDef> pipelines;
    4: map<string, ModelDef> models;
    5: string startingScene;
}