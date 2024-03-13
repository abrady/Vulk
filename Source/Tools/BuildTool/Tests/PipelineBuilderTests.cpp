#define CATCH_CONFIG_MAIN // This tells Catch to provide a main() - only do this in one cpp file
#include <catch.hpp>

#include "../PipelineBuilder.h"
#include "VulkShaderEnums_generated.h"
#include "spirv_cross/spirv_glsl.hpp"
#include <filesystem>
#include <fstream>
#include <iostream>
#include <unordered_map>
#include <vector>

namespace fs = std::filesystem;

static PipelineDeclDef makeTestPipelineDeclDef() {
    PipelineDeclDef def;
    def.name = "TestPipeline";
    def.vertShaderName = "DebugNormals";
    def.geomShaderName = "DebugNormals";
    def.fragShaderName = "DebugNormals";
    def.primitiveTopology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_FAN;
    def.depthTestEnabled = true;
    def.depthWriteEnabled = true;
    def.depthCompareOp = VK_COMPARE_OP_NOT_EQUAL;
    def.cullMode = VK_CULL_MODE_BACK_BIT;
    def.blending = {
        .enabled = true,
        .colorMask = "RB",
    };
    return def;
}

TEST_CASE("PipelineBuilder Tests") { // Define your tests here
    fs::path builtShadersDir = fs::path(__FILE__).parent_path() / "shaders";

    SECTION("Test Basics") {
        ShaderInfo info = PipelineBuilder::getShaderInfo(builtShadersDir / "vert" / "DebugNormals.vertspv");
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
        ShaderInfo info = PipelineBuilder::getShaderInfo(builtShadersDir / "frag" / "GoochShading.fragspv");
        CHECK(info.samplerBindings[VulkShaderTextureBinding_NormalSampler] == "normSampler");
    }
    SECTION("Test mismatch in upstream/downstream") {
        ShaderInfo info1 = PipelineBuilder::getShaderInfo(builtShadersDir / "vert" / "DebugNormals.vertspv");
        ShaderInfo info2 = PipelineBuilder::getShaderInfo(builtShadersDir / "frag" / "GoochShading.fragspv");
        std::string errMsg;
        CHECK(PipelineBuilder::checkConnections(info1, info2, errMsg) == false);
    }
    SECTION("Test match in upstream/downstream") {
        ShaderInfo info1 = PipelineBuilder::getShaderInfo(builtShadersDir / "vert" / "DebugNormals.vertspv");
        ShaderInfo info2 = PipelineBuilder::getShaderInfo(builtShadersDir / "geom" / "DebugNormals.geomspv");
        ShaderInfo info3 = PipelineBuilder::getShaderInfo(builtShadersDir / "frag" / "DebugNormals.fragspv");
        std::string errMsg;
        CAPTURE(errMsg);
        CHECK(PipelineBuilder::checkConnections(info1, info2, errMsg) == true);
        CHECK(PipelineBuilder::checkConnections(info2, info3, errMsg) == true);
    }
    SECTION("Test Pipeline Generation") {
        PipelineDeclDef def = makeTestPipelineDeclDef();
        PipelineDeclDef res = PipelineBuilder::buildPipeline(def, builtShadersDir);
        CHECK(res.name == "TestPipeline");
        CHECK(res.vertShaderName == "DebugNormals");
        CHECK(res.geomShaderName == "DebugNormals");
        CHECK(res.fragShaderName == "DebugNormals");
        CHECK(res.primitiveTopology == VK_PRIMITIVE_TOPOLOGY_TRIANGLE_FAN);
        CHECK(res.depthTestEnabled == true);
        CHECK(res.depthWriteEnabled == true);

        CHECK(res.depthCompareOp == VK_COMPARE_OP_NOT_EQUAL);

        CHECK(res.descriptorSet.uniformBuffers[VK_SHADER_STAGE_VERTEX_BIT] ==
              std::vector<VulkShaderUBOBinding>{VulkShaderUBOBinding_Xforms, VulkShaderUBOBinding_ModelXform, VulkShaderUBOBinding_DebugNormals});
        CHECK(res.descriptorSet.uniformBuffers[VK_SHADER_STAGE_GEOMETRY_BIT] == std::vector<VulkShaderUBOBinding>{});
        CHECK(res.descriptorSet.uniformBuffers[VK_SHADER_STAGE_FRAGMENT_BIT] == std::vector<VulkShaderUBOBinding>{VulkShaderUBOBinding_EyePos});
    }
    SECTION("buildPipelineFile") {
        fs::path builtPipelinesDir = fs::path(__FILE__).parent_path() / "pipelines";
        if (fs::exists(builtPipelinesDir)) {
            std::error_code ec;
            fs::remove_all(builtPipelinesDir, ec);
            CHECK(!ec);
        }
        CHECK(fs::create_directory(builtPipelinesDir));
        PipelineDeclDef def = makeTestPipelineDeclDef();
        fs::path builtPipeline = builtPipelinesDir / "TestPipeline.json";
        PipelineBuilder::buildPipelineFile(def, builtShadersDir, builtPipeline);
        CHECK(fs::exists(builtPipeline));
        nlohmann::json j;
        std::ifstream file(builtPipeline);
        file >> j;
        PipelineDeclDef def2 = PipelineDeclDef::fromJSON(j);
        CHECK(def2.name == def.name);
        CHECK(def2.vertShaderName == def.vertShaderName);
        CHECK(def2.geomShaderName == def.geomShaderName);
        CHECK(def2.fragShaderName == def.fragShaderName);
        CHECK(def2.primitiveTopology == def.primitiveTopology);
        CHECK(def2.depthTestEnabled == def.depthTestEnabled);
        CHECK(def2.depthWriteEnabled == def.depthWriteEnabled);
        CHECK(def2.depthCompareOp == def.depthCompareOp);
        CHECK(def2.blending.enabled == def.blending.enabled);
        CHECK(def2.blending.colorMask == def.blending.colorMask);
        CHECK(def2.blending.getColorMask() == def.blending.getColorMask());
        CHECK(def2.cullMode == def.cullMode);
        CHECK(sizeof(def) == 472); // reminder to add new fields to the test
    }
}
