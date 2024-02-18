#define CATCH_CONFIG_MAIN // This tells Catch to provide a main() - only do this in one cpp file
#include <catch.hpp>

#include "../PipelineBuilder.h"
#include "Vulk/VulkShaderEnums.h"
#include "spirv_cross/spirv_glsl.hpp"
#include <filesystem>
#include <fstream>
#include <iostream>
#include <unordered_map>
#include <vector>

namespace fs = std::filesystem;

TEST_CASE("PipelineBuilder Tests") { // Define your tests here
    SECTION("Test Basics") {
        ShaderInfo info = PipelineBuilder::getShaderInfo(fs::path(__FILE__).parent_path() / "spirvs" / "vert" / "DebugNormals.vertspv");
        CHECK(info.uboBindings[VulkShaderUBOBinding_Xforms] == "UniformBufferObject");
        CHECK(info.uboBindings[VulkShaderUBOBinding_ModelXform] == "ModelXformUBO");
        CHECK(info.uboBindings[VulkShaderUBOBinding_DebugNormals] == "DebugNormalsUBO");
        CHECK(info.inputLocations[VulkVertBindingLocation_PosBinding] == "inPosition");
        CHECK(info.inputLocations[VulkVertBindingLocation_NormalBinding] == "inNormal");
        CHECK(info.inputLocations[VulkVertBindingLocation_TangentBinding] == "inTangent");
        CHECK(info.outputLocations[VulkVertBindingLocation_PosBinding] == "outWorldPos");
        CHECK(info.outputLocations[VulkVertBindingLocation_NormalBinding] == "outWorldNorm");
        CHECK(info.outputLocations[VulkVertBindingLocation_Pos2Binding] == "outProjPos");
    }
    SECTION("Test Gooch Frag") {
        ShaderInfo info = PipelineBuilder::getShaderInfo(fs::path(__FILE__).parent_path() / "spirvs" / "frag" / "GoochShading.fragspv");
        CHECK(info.samplerBindings[VulkShaderTextureBinding_NormalSampler] == "normSampler");
    }
    SECTION("Test mismatch in upstream/downstream") {
        ShaderInfo info1 = PipelineBuilder::getShaderInfo(fs::path(__FILE__).parent_path() / "spirvs" / "vert" / "DebugNormals.vertspv");
        ShaderInfo info2 = PipelineBuilder::getShaderInfo(fs::path(__FILE__).parent_path() / "spirvs" / "frag" / "GoochShading.fragspv");
        std::string errMsg;
        CHECK(PipelineBuilder::checkConnections(info1, info2, errMsg) == false);
    }
    SECTION("Test match in upstream/downstream") {
        ShaderInfo info1 = PipelineBuilder::getShaderInfo(fs::path(__FILE__).parent_path() / "spirvs" / "vert" / "DebugNormals.vertspv");
        ShaderInfo info2 = PipelineBuilder::getShaderInfo(fs::path(__FILE__).parent_path() / "spirvs" / "geom" / "DebugNormals.geomspv");
        ShaderInfo info3 = PipelineBuilder::getShaderInfo(fs::path(__FILE__).parent_path() / "spirvs" / "frag" / "DebugNormals.fragspv");
        std::string errMsg;
        CAPTURE(errMsg);
        CHECK(PipelineBuilder::checkConnections(info1, info2, errMsg) == true);
        CHECK(PipelineBuilder::checkConnections(info2, info3, errMsg) == true);
    }
    SECTION("Test Pipeline Generation") {
        PipelineDef def;
        def.name = "TestPipeline";
        def.vertexShader = std::make_shared<ShaderDef>(ShaderDef{"DebugNormals", std::filesystem::path("/")});
        def.geometryShader = std::make_shared<ShaderDef>(ShaderDef{"DebugNormals", std::filesystem::path("/")});
        def.fragmentShader = std::make_shared<ShaderDef>(ShaderDef{"DebugNormals", std::filesystem::path("/")});
        def.primitiveTopology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
        def.depthTestEnabled = true;
        def.depthWriteEnabled = true;
        def.depthCompareOp = VK_COMPARE_OP_LESS;
        PipelineDef res = PipelineBuilder::buildPipeline(def, fs::path(__FILE__).parent_path() / "spirvs");
        CHECK(res.name == "TestPipeline");
        CHECK(res.vertexShader->name == "DebugNormals");
        CHECK(res.geometryShader->name == "DebugNormals");
        CHECK(res.fragmentShader->name == "DebugNormals");
        CHECK(res.primitiveTopology == VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST);
        CHECK(res.depthTestEnabled == true);
        CHECK(res.depthWriteEnabled == true);
        CHECK(res.depthCompareOp == VK_COMPARE_OP_LESS);
        CHECK(res.descriptorSet.uniformBuffers[VK_SHADER_STAGE_VERTEX_BIT] ==
              std::vector<VulkShaderUBOBindings>{VulkShaderUBOBinding_Xforms, VulkShaderUBOBinding_ModelXform, VulkShaderUBOBinding_DebugNormals});
        CHECK(res.descriptorSet.uniformBuffers[VK_SHADER_STAGE_GEOMETRY_BIT] == std::vector<VulkShaderUBOBindings>{});
        CHECK(res.descriptorSet.uniformBuffers[VK_SHADER_STAGE_FRAGMENT_BIT] == std::vector<VulkShaderUBOBindings>{VulkShaderUBOBinding_EyePos});
    }
}
