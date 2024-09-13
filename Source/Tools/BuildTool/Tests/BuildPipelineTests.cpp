#include <catch.hpp>

#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers.hpp>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <unordered_map>
#include <vector>
#include "../BuildPipeline.h"
#include "spirv_cross/spirv_glsl.hpp"

namespace fs = std::filesystem;
using namespace vulk::cpp2;

static SrcPipelineDef makeTestGeomShaderPipelineDeclDef() {
    SrcPipelineDef def;
    def.name_ref()              = "TestPipeline";
    def.vertShader_ref()        = "DebugNormals";
    def.geomShader_ref()        = "DebugNormals";
    def.fragShader_ref()        = "DebugNormals";
    def.primitiveTopology_ref() = "TriangleFan";
    def.polygonMode_ref()       = "FILL";
    def.depthTestEnabled_ref()  = true;
    def.depthWriteEnabled_ref() = true;
    def.depthCompareOp_ref()    = "NOT_EQUAL";
    def.cullMode_ref().value()  = "BACK";

    // simulate gbufs for the pipeline
    PipelineBlendingDef blending;
    blending.enabled_ref()        = true;
    blending.colorWriteMask_ref() = "RB";
    def.colorBlends_ref()->push_back(blending);
    blending.colorWriteMask_ref() = "G";
    def.colorBlends_ref()->push_back(blending);
    blending.colorWriteMask_ref() = "B";
    def.colorBlends_ref()->push_back(blending);
    def.cullMode_ref() = "BACK";
    return def;
}

static SrcPipelineDef makeTestDeferredLightingPipelineDeclDef() {
    SrcPipelineDef def;
    def.name_ref()       = "TestPipeline";
    def.vertShader_ref() = "DeferredRenderLighting";
    def.fragShader_ref() = "DeferredRenderLighting";
    return def;
}

TEST_CASE("PipelineBuilder Tests") {  // Define your tests here
    fs::path builtShadersDir = fs::path(__FILE__).parent_path() / "shaders";

    SECTION("Test Basics") {
        // int size = sizeof(SrcPipelineDef);
        // std::cout << "Size of SrcPipelineDef: " << size << std::endl;
        ShaderInfo info = PipelineBuilder::getShaderInfo(builtShadersDir / "vert" / "DebugNormals.vertspv");
        CHECK(info.uboBindings[VulkShaderUBOBinding::Xforms] == "UniformBufferObject");
        CHECK(info.uboBindings[VulkShaderUBOBinding::ModelXform] == "ModelXformUBO");
        CHECK(info.uboBindings[VulkShaderUBOBinding::DebugNormals] == "DebugNormalsUBO");
        CHECK(info.inputLocations[VulkShaderLocation::Pos] == "inPosition");
        CHECK(info.inputLocations[VulkShaderLocation::Normal] == "inNormal");
        CHECK(info.inputLocations[VulkShaderLocation::Tangent] == "inTangent");
        CHECK(info.outputLocations[VulkShaderLocation::Pos] == "outWorldPos");
        CHECK(info.outputLocations[VulkShaderLocation::Normal] == "outWorldNorm");
        CHECK(info.outputLocations[VulkShaderLocation::Pos2] == "outProjPos");
    }
    SECTION("Test Gooch Frag") {
        ShaderInfo info = PipelineBuilder::getShaderInfo(builtShadersDir / "frag" / "GoochShading.fragspv");
        CHECK(info.samplerBindings[VulkShaderTextureBinding::NormalSampler] == "normSampler");
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
    SECTION("Test match in upstream/downstream") {
        ShaderInfo info = PipelineBuilder::getShaderInfo(builtShadersDir / "frag" / "DeferredRenderLighting.fragspv");
        CHECK(info.inputAttachments.size() == 4);
        CHECK(info.inputAttachments[GBufBinding::Albedo].atmtIdx == GBufAtmtIdx::Albedo);
        CHECK(info.inputAttachments[GBufBinding::Albedo].name == "albedoMap");
        CHECK(info.inputAttachments[GBufBinding::Depth].atmtIdx == GBufAtmtIdx::Depth);
        CHECK(info.inputAttachments[GBufBinding::Depth].name == "depthMap");
        CHECK(info.inputAttachments[GBufBinding::Normal].atmtIdx == GBufAtmtIdx::Normal);
        CHECK(info.inputAttachments[GBufBinding::Normal].name == "normalMap");
        CHECK(info.inputAttachments[GBufBinding::Material].atmtIdx == GBufAtmtIdx::Material);
        CHECK(TEnumTraits<GBufAtmtIdx>::max() == GBufAtmtIdx::Depth);  // to catch if we add more
    }
    SECTION("Test Pipeline Generation") {
        SrcPipelineDef def          = makeTestGeomShaderPipelineDeclDef();
        vulk::cpp2::PipelineDef res = PipelineBuilder::buildPipeline(def, builtShadersDir);
        CHECK(res.get_name() == "TestPipeline");
        CHECK(res.get_vertShader() == "DebugNormals");
        CHECK(res.get_geomShader() == "DebugNormals");
        CHECK(res.get_fragShader() == "DebugNormals");
        CHECK(res.get_primitiveTopology() == VulkPrimitiveTopology::TriangleFan);
        CHECK(res.get_polygonMode() == VulkPolygonMode::FILL);
        CHECK(res.get_depthTestEnabled() == true);
        CHECK(res.get_depthWriteEnabled() == true);
        auto locs = std::vector<VulkShaderLocation>{VulkShaderLocation::Pos,
                                                    VulkShaderLocation::Normal,
                                                    VulkShaderLocation::Tangent,
                                                    VulkShaderLocation::TexCoord};
        CHECK(res.get_vertInputs() == locs);

        CHECK(res.get_depthCompareOp() == VulkCompareOp::NOT_EQUAL);

        auto loc2  = std::vector<VulkShaderUBOBinding>{VulkShaderUBOBinding::Xforms,
                                                       VulkShaderUBOBinding::ModelXform,
                                                       VulkShaderUBOBinding::DebugNormals};
        auto dsdef = res.get_descriptorSetDef();
        auto ub    = dsdef.get_uniformBuffers();
        CHECK(ub.at(VK_SHADER_STAGE_VERTEX_BIT) == loc2);
        CHECK(ub.count(VK_SHADER_STAGE_GEOMETRY_BIT) == 0);
        CHECK(ub.at(VK_SHADER_STAGE_FRAGMENT_BIT) == std::vector<VulkShaderUBOBinding>{VulkShaderUBOBinding::EyePos});
    }

    SECTION("Test Pipeline Generation Deferred Lighting") {
        SrcPipelineDef def          = makeTestDeferredLightingPipelineDeclDef();
        vulk::cpp2::PipelineDef res = PipelineBuilder::buildPipeline(def, builtShadersDir);
        CHECK(res.get_name() == "TestPipeline");
        CHECK(res.get_vertShader() == "DeferredRenderLighting");
        CHECK(res.get_fragShader() == "DeferredRenderLighting");

        // check input attachments
        auto dsdef            = res.get_descriptorSetDef();
        auto inputAttachments = dsdef.get_inputAttachments();
        CHECK(inputAttachments.count(VK_SHADER_STAGE_FRAGMENT_BIT) == 1);
        CHECK(inputAttachments.size() == 1);
        auto& attmts = inputAttachments.at(VK_SHADER_STAGE_FRAGMENT_BIT);
        CHECK(attmts.size() == 4);
        CHECK(attmts[0].get_binding() == GBufBinding::Albedo);
        CHECK(attmts[0].get_atmtIdx() == GBufAtmtIdx::Albedo);
        CHECK(attmts[1].get_binding() == GBufBinding::Material);
        CHECK(attmts[1].get_atmtIdx() == GBufAtmtIdx::Material);
        // etc...
    }

    SECTION("buildPipelineFile") {
        fs::path builtPipelinesDir = fs::path(__FILE__).parent_path() / "build" / "pipelines";
        if (fs::exists(builtPipelinesDir)) {
            std::error_code ec;
            fs::remove_all(builtPipelinesDir, ec);
            CHECK(!ec);
        }
        CHECK(fs::create_directories(builtPipelinesDir));
        SrcPipelineDef def     = makeTestGeomShaderPipelineDeclDef();
        fs::path builtPipeline = builtPipelinesDir / "TestPipeline.pipeline";
        PipelineBuilder::buildPipelineFile(def, builtShadersDir, builtPipeline);
        CHECK(fs::exists(builtPipeline));
        vulk::cpp2::PipelineDef builtDef;
        readDefFromFile(builtPipeline.string(), builtDef);
        CHECK(builtDef.get_name() == def.get_name());
        CHECK(builtDef.get_vertShader() == def.get_vertShader());
        CHECK(builtDef.get_geomShader() == def.get_geomShader());
        CHECK(builtDef.get_fragShader() == def.get_fragShader());
        CHECK(builtDef.get_primitiveTopology() ==
              apache::thrift::util::enumValueOrThrow<decltype(builtDef.get_primitiveTopology())>(def.get_primitiveTopology()));
        CHECK(builtDef.get_polygonMode() ==
              apache::thrift::util::enumValueOrThrow<decltype(builtDef.get_polygonMode())>(def.get_polygonMode()));
        CHECK(builtDef.get_depthTestEnabled() == def.get_depthTestEnabled());
        CHECK(builtDef.get_depthWriteEnabled() == def.get_depthWriteEnabled());
        CHECK(builtDef.get_depthCompareOp() ==
              apache::thrift::util::enumValueOrThrow<decltype(builtDef.get_depthCompareOp())>(def.get_depthCompareOp()));
        CHECK(builtDef.get_colorBlends().size() == def.get_colorBlends().size());
        for (size_t i = 0; i < def.get_colorBlends().size(); i++) {
            CHECK(builtDef.get_colorBlends()[i].get_enabled() == def.get_colorBlends()[i].get_enabled());
            CHECK(builtDef.get_colorBlends()[i].get_colorWriteMask() == def.get_colorBlends()[i].get_colorWriteMask());
        }
        CHECK(builtDef.get_cullMode() == apache::thrift::util::enumValueOrThrow<VulkCullModeFlags>(def.get_cullMode()));
        auto v = std::vector<VulkShaderLocation>{VulkShaderLocation::Pos,
                                                 VulkShaderLocation::Normal,
                                                 VulkShaderLocation::Tangent,
                                                 VulkShaderLocation::TexCoord};
        CHECK(builtDef.get_vertInputs() == v);
        CHECK(builtDef.get_pushConstants().size() == 1);
        CHECK(builtDef.get_pushConstants()[0].get_stageFlags() ==
              (VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT | VK_SHADER_STAGE_GEOMETRY_BIT));
        CHECK(builtDef.get_pushConstants()[0].get_size() == 4);

        REQUIRE(sizeof(def) == 392);       // reminder to add new fields to the test
        REQUIRE(sizeof(builtDef) == 416);  // reminder to add new fields to the test
        // I would do a static assert here but it doesn't print out the sizes.
        auto v2 = std::vector<VulkShaderUBOBinding>{VulkShaderUBOBinding::Xforms,
                                                    VulkShaderUBOBinding::ModelXform,
                                                    VulkShaderUBOBinding::DebugNormals};
        CHECK(builtDef.get_descriptorSetDef().get_uniformBuffers().at(VK_SHADER_STAGE_VERTEX_BIT) == v2);
        CHECK(builtDef.get_descriptorSetDef().get_uniformBuffers().at(VK_SHADER_STAGE_FRAGMENT_BIT) ==
              std::vector<VulkShaderUBOBinding>{VulkShaderUBOBinding::EyePos});
        CHECK(builtDef.get_descriptorSetDef().get_imageSamplers().at(VK_SHADER_STAGE_VERTEX_BIT) ==
              std::vector<VulkShaderTextureBinding>{VulkShaderTextureBinding::NormalSampler});
    }
}
