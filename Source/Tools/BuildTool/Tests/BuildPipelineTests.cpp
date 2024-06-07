#include <catch.hpp>

#include "../BuildPipeline.h"
#include "spirv_cross/spirv_glsl.hpp"
#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers.hpp>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <unordered_map>
#include <vector>

namespace fs = std::filesystem;

static vulk::cpp2::BuiltPipelineDef makeTestPipelineDeclDef() {
    vulk::cpp2::BuiltPipelineDef def;
    def.name_ref() = "TestPipeline";
    def.vertShaderName_ref() = "DebugNormals";
    def.geomShaderName_ref() = "DebugNormals";
    def.fragShaderName_ref() = "DebugNormals";
    def.primitiveTopology_ref() = vulk::cpp2::VulkPrimitiveTopology::TriangleFan;
    def.polygonMode_ref() = vulk::cpp2::VulkPolygonMode::FILL;
    def.depthTestEnabled_ref() = true;
    def.depthWriteEnabled_ref() = true;
    def.depthCompareOp_ref() = vulk::cpp2::VulkCompareOp::NOT_EQUAL;
    def.cullMode_ref() = VK_CULL_MODE_BACK_BIT;
    def.blending_ref()->enabled_ref() = true;
    def.blending_ref()->colorWriteMask_ref() = "RB";
    return def;
}

TEST_CASE("PipelineBuilder Tests") { // Define your tests here
    fs::path builtShadersDir = fs::path(__FILE__).parent_path() / "shaders";

    SECTION("Test Basics") {
        ShaderInfo info = PipelineBuilder::getShaderInfo(builtShadersDir / "vert" / "DebugNormals.vertspv");
        CHECK(info.uboBindings[vulk::cpp2::VulkShaderUBOBinding::Xforms] == "UniformBufferObject");
        CHECK(info.uboBindings[vulk::cpp2::VulkShaderUBOBinding::ModelXform] == "ModelXformUBO");
        CHECK(info.uboBindings[vulk::cpp2::VulkShaderUBOBinding::DebugNormals] == "DebugNormalsUBO");
        CHECK(info.inputLocations[vulk::cpp2::VulkShaderLocation::Pos] == "inPosition");
        CHECK(info.inputLocations[vulk::cpp2::VulkShaderLocation::Normal] == "inNormal");
        CHECK(info.inputLocations[vulk::cpp2::VulkShaderLocation::Tangent] == "inTangent");
        CHECK(info.outputLocations[vulk::cpp2::VulkShaderLocation::Pos] == "outWorldPos");
        CHECK(info.outputLocations[vulk::cpp2::VulkShaderLocation::Normal] == "outWorldNorm");
        CHECK(info.outputLocations[vulk::cpp2::VulkShaderLocation::Pos2] == "outProjPos");
    }
    SECTION("Test Gooch Frag") {
        ShaderInfo info = PipelineBuilder::getShaderInfo(builtShadersDir / "frag" / "GoochShading.fragspv");
        CHECK(info.samplerBindings[vulk::cpp2::VulkShaderTextureBinding::NormalSampler] == "normSampler");
    }
    SECTION("Test Pick Frag") {
        ShaderInfo info = PipelineBuilder::getShaderInfo(builtShadersDir / "frag" / "pick.fragspv");
        CHECK(info.pushConstants.size() == 1);
        CHECK(info.pushConstants[0] == 4);
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
        vulk::cpp2::BuiltPipelineDef def = makeTestPipelineDeclDef();
        vulk::cpp2::BuiltPipelineDef res = PipelineBuilder::buildPipeline(def, builtShadersDir);
        CHECK(res.get_name() == "TestPipeline");
        CHECK(res.get_vertShaderName() == "DebugNormals");
        CHECK(res.get_geomShaderName() == "DebugNormals");
        CHECK(res.get_fragShaderName() == "DebugNormals");
        CHECK(res.get_primitiveTopology() == vulk::cpp2::VulkPrimitiveTopology::TriangleFan);
        CHECK(res.get_polygonMode() == vulk::cpp2::VulkPolygonMode::FILL);
        CHECK(res.get_depthTestEnabled() == true);
        CHECK(res.get_depthWriteEnabled() == true);
        auto locs = std::vector<vulk::cpp2::VulkShaderLocation>{vulk::cpp2::VulkShaderLocation::Pos, vulk::cpp2::VulkShaderLocation::Normal,
                                                                vulk::cpp2::VulkShaderLocation::Tangent, vulk::cpp2::VulkShaderLocation::TexCoord};
        CHECK(res.get_vertInputs() == locs);

        CHECK(res.get_depthCompareOp() == vulk::cpp2::VulkCompareOp::NOT_EQUAL);

        auto loc2 = std::vector<vulk::cpp2::VulkShaderUBOBinding>{vulk::cpp2::VulkShaderUBOBinding::Xforms, vulk::cpp2::VulkShaderUBOBinding::ModelXform,
                                                                  vulk::cpp2::VulkShaderUBOBinding::DebugNormals};
        auto ub = res.get_descriptorSetDef().get_uniformBuffers();
        CHECK(ub.at(VK_SHADER_STAGE_VERTEX_BIT) == loc2);
        CHECK(ub.count(VK_SHADER_STAGE_GEOMETRY_BIT) == 0);
        CHECK(ub.at(VK_SHADER_STAGE_FRAGMENT_BIT) == std::vector<vulk::cpp2::VulkShaderUBOBinding>{vulk::cpp2::VulkShaderUBOBinding::EyePos});
    }

    SECTION("buildPipelineFile") {
        fs::path builtPipelinesDir = fs::path(__FILE__).parent_path() / "build" / "pipelines";
        if (fs::exists(builtPipelinesDir)) {
            std::error_code ec;
            fs::remove_all(builtPipelinesDir, ec);
            CHECK(!ec);
        }
        CHECK(fs::create_directories(builtPipelinesDir));
        vulk::cpp2::BuiltPipelineDef def = makeTestPipelineDeclDef();
        fs::path builtPipeline = builtPipelinesDir / "TestPipeline.pipeline";
        PipelineBuilder::buildPipelineFile(def, builtShadersDir, builtPipeline);
        CHECK(fs::exists(builtPipeline));
        nlohmann::json j;
        vulk::cpp2::BuiltPipelineDef builtDef;
        readDefFromFile(builtPipeline.string(), builtDef);
        CHECK(builtDef.get_name() == def.get_name());
        CHECK(builtDef.get_vertShaderName() == def.get_vertShaderName());
        CHECK(builtDef.get_geomShaderName() == def.get_geomShaderName());
        CHECK(builtDef.get_fragShaderName() == def.get_fragShaderName());
        CHECK(builtDef.get_primitiveTopology() == def.get_primitiveTopology());
        CHECK(builtDef.get_polygonMode() == def.get_polygonMode());
        CHECK(builtDef.get_polygonMode() == def.get_polygonMode());
        CHECK(builtDef.get_depthTestEnabled() == def.get_depthTestEnabled());
        CHECK(builtDef.get_depthWriteEnabled() == def.get_depthWriteEnabled());
        CHECK(builtDef.get_depthCompareOp() == def.get_depthCompareOp());
        CHECK(builtDef.get_blending().get_enabled() == def.get_blending().get_enabled());
        CHECK(builtDef.get_blending().get_colorWriteMask() == def.get_blending().get_colorWriteMask());
        CHECK(builtDef.get_cullMode() == def.get_cullMode());
        auto v = std::vector<vulk::cpp2::VulkShaderLocation>{vulk::cpp2::VulkShaderLocation::Pos, vulk::cpp2::VulkShaderLocation::Normal, vulk::cpp2::VulkShaderLocation::Tangent,
                                                             vulk::cpp2::VulkShaderLocation::TexCoord};
        CHECK(builtDef.get_vertInputs() == v);
        CHECK(builtDef.get_pushConstants().size() == 1);
        CHECK(builtDef.get_pushConstants()[0].get_stageFlags() == (VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT | VK_SHADER_STAGE_GEOMETRY_BIT));
        CHECK(builtDef.get_pushConstants()[0].get_size() == 4);

        REQUIRE(sizeof(def) == 472);      // reminder to add new fields to the test
        REQUIRE(sizeof(builtDef) == 472); // reminder to add new fields to the test
        // I would do a static assert here but it doesn't print out the sizes.
        auto v2 = std::vector<vulk::cpp2::VulkShaderUBOBinding>{vulk::cpp2::VulkShaderUBOBinding::Xforms, vulk::cpp2::VulkShaderUBOBinding::ModelXform,
                                                                vulk::cpp2::VulkShaderUBOBinding::DebugNormals};
        CHECK(builtDef.get_descriptorSetDef().get_uniformBuffers().at(VK_SHADER_STAGE_VERTEX_BIT) == v2);
        CHECK(builtDef.get_descriptorSetDef().get_uniformBuffers().at(VK_SHADER_STAGE_FRAGMENT_BIT) ==
              std::vector<vulk::cpp2::VulkShaderUBOBinding>{vulk::cpp2::VulkShaderUBOBinding::EyePos});
        CHECK(builtDef.get_descriptorSetDef().get_imageSamplers().at(VK_SHADER_STAGE_VERTEX_BIT) ==
              std::vector<vulk::cpp2::VulkShaderTextureBinding>{vulk::cpp2::VulkShaderTextureBinding::NormalSampler});
    }
}
