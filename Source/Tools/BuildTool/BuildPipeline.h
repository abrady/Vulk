#pragma once

#include "spirv_cross/spirv_glsl.hpp"
#include <filesystem>
#include <fstream>
#include <iostream>
#include <thrift/lib/cpp/util/EnumUtils.h>
#include <unordered_map>
#include <vector>

#include "Vulk/VulkResourceMetadata.h"

#include <nlohmann/json.hpp>

struct ShaderInfo {
    std::string name;
    std::string entryPoint;
    std::unordered_map<vulk::cpp2::VulkShaderUBOBinding, std::string> uboBindings;
    std::unordered_map<vulk::cpp2::VulkShaderSSBOBinding, std::string> sboBindings;
    std::unordered_map<vulk::cpp2::VulkShaderTextureBinding, std::string> samplerBindings;
    std::unordered_map<vulk::cpp2::VulkShaderLocation, std::string> inputLocations;
    std::unordered_map<vulk::cpp2::VulkShaderLocation, std::string> outputLocations;
    std::vector<uint32_t> pushConstants; // bitfield of vulk::cpp2::VulkShaderStage
};

class PipelineBuilder {

    static std::shared_ptr<spdlog::logger> logger() {
        static std::shared_ptr<spdlog::logger> logger;
        static std::once_flag flag;
        std::call_once(flag, []() {
            logger = VulkLogger::CreateLogger("PipelineBuilder");
            // logger->set_level(spdlog::level::trace);
        });
        return logger;
    }

public:
#pragma warning(push)
#pragma warning(disable : 6262) // Function uses '18964' bytes of stack. - the SPIRV structs are big, not a problem.
    static ShaderInfo getShaderInfo(std::filesystem::path shaderPath) {

        auto spirvData = readSPIRVFile(shaderPath);
        spirv_cross::CompilerGLSL glsl(std::move(spirvData));
        const spirv_cross::ShaderResources resources = glsl.get_shader_resources();

        ShaderInfo parsedShader;
        parsedShader.name = shaderPath.stem().string();
        parsedShader.entryPoint = glsl.get_entry_points_and_stages()[0].name;

        // vulk::cpp2::VulkShaderStage shaderStage;
        // switch (glsl.get_execution_model()) {
        // case spv::ExecutionModelVertex:
        //     logger()->trace("Vertex Shader");
        //     shaderStage = VulkShaderStage_VERTEX;
        //     break;
        // case spv::ExecutionModelGeometry:
        //     logger()->trace("Geometry Shader");
        //     shaderStage = VulkShaderStage_GEOMETRY;
        //     break;
        // case spv::ExecutionModelFragment:
        //     logger()->trace("Fragment Shader");
        //     shaderStage = VulkShaderStage_FRAGMENT;
        //     break;
        // case spv::ExecutionModelTessellationControl:
        //     logger()->trace("Tessellation Control Shader");
        //     shaderStage = VulkShaderStage_TESSELLATION_CONTROL;
        //     break;
        // case spv::ExecutionModelTessellationEvaluation:
        //     logger()->trace("Tessellation Evaluation Shader");
        //     shaderStage = VulkShaderStage_TESSELLATION_EVALUATION;
        //     break;
        // case spv::ExecutionModelGLCompute: // GL compute?
        //     logger()->trace("Compute Shader");
        //     shaderStage = VulkShaderStage_COMPUTE;
        //     break;
        // default:
        //     VULK_THROW_FMT("Unsupported shader stage: {}", (int)glsl.get_execution_model());
        // }

        // For UBOs
        for (const spirv_cross::Resource& resource : resources.uniform_buffers) {
            // unsigned set = glsl.get_decoration(resource.id, spv::DecorationDescriptorSet);
            vulk::cpp2::VulkShaderUBOBinding binding = (vulk::cpp2::VulkShaderUBOBinding)glsl.get_decoration(resource.id, spv::DecorationBinding);
            parsedShader.uboBindings[binding] = resource.name;
        }

        // For SBOs
        for (const spirv_cross::Resource& resource : resources.storage_buffers) {
            // unsigned set = glsl.get_decoration(resource.id, spv::DecorationDescriptorSet);
            vulk::cpp2::VulkShaderSSBOBinding binding = (vulk::cpp2::VulkShaderSSBOBinding)glsl.get_decoration(resource.id, spv::DecorationBinding);
            parsedShader.sboBindings[binding] = resource.name;
        }

        // For Samplers
        for (const spirv_cross::Resource& resource : resources.sampled_images) {
            // unsigned set = glsl.get_decoration(resource.id, spv::DecorationDescriptorSet);
            vulk::cpp2::VulkShaderTextureBinding binding = (vulk::cpp2::VulkShaderTextureBinding)glsl.get_decoration(resource.id, spv::DecorationBinding);
            parsedShader.samplerBindings[binding] = resource.name;
        }

        // Inputs
        for (const spirv_cross::Resource& resource : resources.stage_inputs) {
            vulk::cpp2::VulkShaderLocation location = (vulk::cpp2::VulkShaderLocation)glsl.get_decoration(resource.id, spv::DecorationLocation);
            parsedShader.inputLocations[location] = resource.name;
        }

        // Outputs
        for (const spirv_cross::Resource& resource : resources.stage_outputs) {
            vulk::cpp2::VulkShaderLocation location = (vulk::cpp2::VulkShaderLocation)glsl.get_decoration(resource.id, spv::DecorationLocation);
            parsedShader.outputLocations[location] = resource.name;
        }

        // Push constants
        for (const spirv_cross::Resource& resource : resources.push_constant_buffers) {
            // Get the buffer size
            spirv_cross::SPIRType const& type = glsl.get_type(resource.base_type_id);
            uint32_t size = (uint32_t)glsl.get_declared_struct_size(type);
            // Note: Push constants do not have a traditional binding location like uniforms yet.
            unsigned location = 0;
            if (glsl.has_decoration(resource.id, spv::DecorationLocation)) {
                location = glsl.get_decoration(resource.id, spv::DecorationLocation);
            }
            logger()->trace("Push constant buffer: name={}, size={}, location={}", resource.name, size, location);
            VULK_ASSERT(location < 32, "Push constant location is too large"); // no idea what the max may eventually be but this isn't crazy
            if (parsedShader.pushConstants.size() <= location) {
                parsedShader.pushConstants.resize(location + 1);
            }
            parsedShader.pushConstants[location] = size;
        }

        return parsedShader;
    }
#pragma warning(pop)
    static bool checkConnections(ShaderInfo& upstream, ShaderInfo& downstream, std::string& errMsg) {
        errMsg = "";
        // Check if the downstream shader has any inputs that are not outputs of the upstream shader
        for (auto& input : downstream.inputLocations) {
            if (upstream.outputLocations.find(input.first) == upstream.outputLocations.end()) {
                errMsg += "Downstream shader " + downstream.name + " has input " + input.second + " that is not an output of the upstream shader " + upstream.name + "\n";
            }
        }
        for (auto& output : upstream.outputLocations) {
            if (downstream.inputLocations.find(output.first) == downstream.inputLocations.end()) {
                errMsg += "Upstream shader " + upstream.name + " has output " + output.second + " that is not an input of the downstream shader " + downstream.name + "\n";
            }
        }
        return errMsg.empty();
    }

    static ShaderInfo infoFromShader(std::string name, std::string type, std::filesystem::path shadersDir) {
        return getShaderInfo(shadersDir / type / (name + "." + type + "spv"));
    }

    static VkShaderStageFlagBits getShaderStageFromStr(std::string s) {
        static unordered_map<string, VkShaderStageFlagBits> shaderStageFromStr{
            {"vert", VK_SHADER_STAGE_VERTEX_BIT},
            {"frag", VK_SHADER_STAGE_FRAGMENT_BIT},
            {"geom", VK_SHADER_STAGE_GEOMETRY_BIT},
        };

        return shaderStageFromStr.at(s);
    }

    // e.g. the "vert" or "frag" part of the descriptor set
    static void updatePipelineDef(ShaderInfo info, std::string stage, vulk::cpp2::PipelineDef& bp) {
        vulk::cpp2::DescriptorSetDef& def = bp.descriptorSetDef_ref().value();
        int32_t stageFlag = (int)getShaderStageFromStr(stage);
        for (auto& ubo : info.uboBindings) {
            auto& v = def.uniformBuffers_ref()[stageFlag];
            v.push_back((vulk::cpp2::VulkShaderUBOBinding)ubo.first);
        }
        for (auto& sbo : info.sboBindings) {
            def.storageBuffers_ref()[stageFlag].push_back((vulk::cpp2::VulkShaderSSBOBinding)sbo.first);
        }
        for (auto& sampler : info.samplerBindings) {
            auto imageSamplersRef = def.imageSamplers_ref(); // Get the field_ref
            std::map<std::int32_t, std::vector<::vulk::cpp2::VulkShaderTextureBinding>>& samplers = *imageSamplersRef;
            samplers[stageFlag].push_back((vulk::cpp2::VulkShaderTextureBinding)sampler.first);
            // def.imageSamplers_ref()[stageFlag].push_back((vulk::cpp2::VulkShaderBinding)sampler.first);
        }

        for (uint32_t i = 0; i < info.pushConstants.size(); i++) {
            if (i >= bp.get_pushConstants().size()) {
                bp.pushConstants_ref()->resize(i + 1);
                vulk::cpp2::PushConstantDef pc = {};
                pc.stageFlags_ref() = (uint32_t)stageFlag;
                pc.size_ref() = info.pushConstants[i];
                bp.pushConstants_ref()[i] = pc;
            } else {
                auto const& m = bp.get_pushConstants();
                VULK_ASSERT(m[i].get_size() == (int)info.pushConstants[i], "Push constant size mismatch");
                uint32_t flags = (*bp.pushConstants_ref())[i].get_stageFlags();
                flags |= (uint32_t)stageFlag;
                (*bp.pushConstants_ref())[i].stageFlags_ref() = flags;
            }
        }
    }

    static vulk::cpp2::PipelineDef buildPipeline(vulk::cpp2::SrcPipelineDef pipelineIn, std::filesystem::path builtShadersDir) {
        if (!std::filesystem::exists(builtShadersDir)) {
            std::cerr << "Shaders directory does not exist: " << builtShadersDir << std::endl;
            VULK_THROW("PipelineBuilder: Shaders directory does not exist");
        }

        vulk::cpp2::PipelineDef pipelineOut;
        // 1: i32 version;
        // 2: string name;
        // 3: string vertShader;
        // 4: string geomShader;
        // 5: string fragShader;
        // 6: string primitiveTopology; // VulkShaderEnums.VulkPrimitiveTopology
        // 7: bool depthTestEnabled;
        // 8: bool depthWriteEnabled;
        // 9: string depthCompareOp; // VulkShaderEnums.VulkCompareOp
        // 10: string polygonMode; // VulkShaderEnums.VulkPolygonMode
        // 11: i32 cullMode; // VkCullModeFlags
        // 12: PipelineBlendingDef blending;

        pipelineOut.version_ref() = 1;
        pipelineOut.name_ref() = pipelineIn.get_name();
        pipelineOut.vertShader_ref() = pipelineIn.get_vertShader();
        pipelineOut.geomShader_ref() = pipelineIn.get_geomShader();
        pipelineOut.fragShader_ref() = pipelineIn.get_fragShader();
        apache::thrift::util::tryParseEnum(pipelineIn.get_primitiveTopology(), &pipelineOut.primitiveTopology_ref().value());
        pipelineOut.depthTestEnabled_ref() = pipelineIn.get_depthTestEnabled();
        pipelineOut.depthWriteEnabled_ref() = pipelineIn.get_depthWriteEnabled();
        apache::thrift::util::tryParseEnum(pipelineIn.get_depthCompareOp(), &pipelineOut.depthCompareOp_ref().value());
        apache::thrift::util::tryParseEnum(pipelineIn.get_polygonMode(), &pipelineOut.polygonMode_ref().value());
        pipelineOut.cullMode_ref() = (int)apache::thrift::util::enumValueOrThrow<vulk::cpp2::VulkCullModeFlags>(pipelineIn.get_cullMode());
        pipelineOut.blending_ref() = pipelineIn.get_blending();
        static_assert(sizeof(pipelineIn) == 432);

        std::vector<ShaderInfo> shaderInfos;
        ShaderInfo vertShaderInfo = infoFromShader(pipelineIn.get_vertShader(), "vert", builtShadersDir);
        shaderInfos.push_back(vertShaderInfo);
        updatePipelineDef(vertShaderInfo, "vert", pipelineOut);

        if (!pipelineIn.get_geomShader().empty()) {
            ShaderInfo info = infoFromShader(pipelineIn.get_geomShader(), "geom", builtShadersDir);
            shaderInfos.push_back(info);
            updatePipelineDef(info, "geom", pipelineOut);
        }
        if (!pipelineIn.get_fragShader().empty()) {
            ShaderInfo info = infoFromShader(pipelineIn.get_fragShader(), "frag", builtShadersDir);
            shaderInfos.push_back(info);
            updatePipelineDef(info, "frag", pipelineOut);
        }

        for (auto& [location, name] : vertShaderInfo.inputLocations) {
            pipelineOut.vertInputs_ref()->push_back(location);
        }

        std::string errMsgOut;
        for (size_t i = 0; i < shaderInfos.size() - 1; i++) {
            std::string errMsg;
            if (!checkConnections(shaderInfos[i], shaderInfos[i + 1], errMsg)) {
                errMsgOut += "\n";
                errMsgOut += errMsg;
            }
        }
        if (!errMsgOut.empty()) {
            VULK_THROW(errMsgOut);
        }

        return pipelineOut;
    }

    static void buildPipelineFile(vulk::cpp2::SrcPipelineDef pipelineIn, std::filesystem::path builtShadersDir, std::filesystem::path pipelineFileOut) {
        if (!std::filesystem::exists(builtShadersDir)) {
            std::cerr << "Shaders directory does not exist: " << builtShadersDir << std::endl;
            VULK_THROW("PipelineBuilder: Shaders directory does not exist");
        }
        if (!std::filesystem::exists(pipelineFileOut.parent_path())) {
            std::cerr << "Output directory does not exist: " << pipelineFileOut.parent_path() << std::endl;
            VULK_THROW("PipelineBuilder: Output directory does not exist");
        }

        vulk::cpp2::PipelineDef pipelineOut = buildPipeline(pipelineIn, builtShadersDir);
        writeDefToFile(pipelineFileOut.string(), pipelineOut);
    }

    static void buildPipelineFromFile(std::filesystem::path builtShadersDir, std::filesystem::path pipelineFileOut, fs::path pipelineFileSrc) {
        if (!std::filesystem::exists(pipelineFileSrc)) {
            std::cerr << "Pipeline file does not exist: '" << pipelineFileSrc << "'" << std::endl;
            VULK_THROW("PipelineBuilder: Pipeline file does not exist");
        }
        if (!std::filesystem::exists(builtShadersDir)) {
            std::cerr << "Shaders directory does not exist: " << builtShadersDir << std::endl;
            VULK_THROW("PipelineBuilder: Shaders directory does not exist");
        }
        if (!std::filesystem::exists(pipelineFileOut.parent_path())) {
            std::cerr << "Output directory does not exist: " << pipelineFileOut << std::endl;
            VULK_THROW("PipelineBuilder: Output directory does not exist");
        }

        vulk::cpp2::SrcPipelineDef def;
        readDefFromFile(pipelineFileSrc.string(), def);
        buildPipelineFile(def, builtShadersDir, pipelineFileOut);
    }

private:
    static std::vector<uint32_t> readSPIRVFile(std::filesystem::path filename) {
        std::ifstream file(filename, std::ios::ate | std::ios::binary);

        if (!file.is_open()) {
            VULK_THROW("Failed to open file " + filename.string());
        }

        size_t fileSize = (size_t)file.tellg();
        std::vector<uint32_t> buffer(fileSize / sizeof(uint32_t));

        file.seekg(0);
        file.read((char*)buffer.data(), fileSize);
        file.close();

        return buffer;
    }
};