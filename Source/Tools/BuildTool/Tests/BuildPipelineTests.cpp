#include <catch.hpp>

#include "../BuildPipeline.h"
#include "VulkResourceMetadata_generated.h"
#include "VulkShaderEnums_generated.h"
#include "spirv_cross/spirv_glsl.hpp"
#include <filesystem>
#include <fstream>
#include <iostream>
#include <unordered_map>
#include <vector>

namespace fs = std::filesystem;

static SourcePipelineDef makeTestPipelineDeclDef() {
    SourcePipelineDef def;
    def.name = "TestPipeline";
    def.vertShaderName = "DebugNormals";
    def.geomShaderName = "DebugNormals";
    def.fragShaderName = "DebugNormals";
    def.primitiveTopology = VulkPrimitiveTopology_TriangleFan;
    def.depthTestEnabled = true;
    def.depthWriteEnabled = true;
    def.depthCompareOp = VulkCompareOp_NOT_EQUAL;
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
        SourcePipelineDef def = makeTestPipelineDeclDef();
        BuiltPipelineDef res = PipelineBuilder::buildPipeline(def, builtShadersDir);
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
        VulkCereal::inst()->useJSON = false;
        do {
            VulkCereal::inst()->useJSON = !VulkCereal::inst()->useJSON;

            fs::path builtPipelinesDir = fs::path(__FILE__).parent_path() / "build" / "pipelines";
            if (fs::exists(builtPipelinesDir)) {
                std::error_code ec;
                fs::remove_all(builtPipelinesDir, ec);
                CHECK(!ec);
            }
            CHECK(fs::create_directory(builtPipelinesDir));
            SourcePipelineDef def = makeTestPipelineDeclDef();
            fs::path builtPipeline = VulkCereal::inst()->addFileExtension(builtPipelinesDir / "TestPipeline");
            PipelineBuilder::buildPipelineFile(def, builtShadersDir, builtPipeline);
            CHECK(fs::exists(builtPipeline));
            nlohmann::json j;
            BuiltPipelineDef def2;
            VulkCereal::inst()->fromFile(builtPipeline, def2);
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
            CHECK(sizeof(def) == 224);  // reminder to add new fields to the test
            CHECK(sizeof(def2) == 512); // reminder to add new fields to the test
            CHECK(def2.descriptorSet.uniformBuffers[VK_SHADER_STAGE_VERTEX_BIT] ==
                  std::vector<VulkShaderUBOBinding>{VulkShaderUBOBinding_Xforms, VulkShaderUBOBinding_ModelXform, VulkShaderUBOBinding_DebugNormals});
            CHECK(def2.descriptorSet.uniformBuffers[VK_SHADER_STAGE_FRAGMENT_BIT] == std::vector<VulkShaderUBOBinding>{VulkShaderUBOBinding_EyePos});
            CHECK(def2.descriptorSet.imageSamplers[VK_SHADER_STAGE_VERTEX_BIT] ==
                  std::vector<VulkShaderTextureBinding>{VulkShaderTextureBinding_NormalSampler});
        } while (VulkCereal::inst()->useJSON);
    }
}
