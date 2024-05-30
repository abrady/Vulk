#pragma once

#include "spirv_cross/spirv_glsl.hpp"
#include <cereal/archives/binary.hpp>
#include <cereal/archives/json.hpp>
#include <cereal/cereal.hpp>
#include <cereal/types/memory.hpp>
#include <cereal/types/vector.hpp>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <unordered_map>
#include <vector>

#include "Vulk/VulkResourceMetadata.h"
#include "VulkShaderEnums_generated.h"

#include <nlohmann/json.hpp>

struct ShaderInfo {
    std::string name;
    std::string entryPoint;
    std::unordered_map<VulkShaderUBOBinding, std::string> uboBindings;
    std::unordered_map<VulkShaderSSBOBinding, std::string> sboBindings;
    std::unordered_map<VulkShaderTextureBinding, std::string> samplerBindings;
    std::unordered_map<VulkShaderLocation, std::string> inputLocations;
    std::unordered_map<VulkShaderLocation, std::string> outputLocations;
    std::vector<uint32_t> pushConstants; // bitfield of VulkShaderStage
};

class PipelineBuilder {

    static std::shared_ptr<spdlog::logger> logger() {
        static std::shared_ptr<spdlog::logger> logger;
        static std::once_flag flag;
        std::call_once(flag, []() {
            logger = VulkLogger::CreateLogger("PipelineBuilder");
            logger->set_level(spdlog::level::trace);
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

        VulkShaderStage shaderStage;
        switch (glsl.get_execution_model()) {
        case spv::ExecutionModelVertex:
            logger()->trace("Vertex Shader");
            shaderStage = VulkShaderStage_VERTEX;
            break;
        case spv::ExecutionModelGeometry:
            logger()->trace("Geometry Shader");
            shaderStage = VulkShaderStage_GEOMETRY;
            break;
        case spv::ExecutionModelFragment:
            logger()->trace("Fragment Shader");
            shaderStage = VulkShaderStage_FRAGMENT;
            break;
        case spv::ExecutionModelTessellationControl:
            logger()->trace("Tessellation Control Shader");
            shaderStage = VulkShaderStage_TESSELLATION_CONTROL;
            break;
        case spv::ExecutionModelTessellationEvaluation:
            logger()->trace("Tessellation Evaluation Shader");
            shaderStage = VulkShaderStage_TESSELLATION_EVALUATION;
            break;
        case spv::ExecutionModelGLCompute: // GL compute?
            logger()->trace("Compute Shader");
            shaderStage = VulkShaderStage_COMPUTE;
            break;
        default:
            VULK_THROW_FMT("Unsupported shader stage: {}", (int)glsl.get_execution_model());
        }

        // For UBOs
        for (const spirv_cross::Resource& resource : resources.uniform_buffers) {
            // unsigned set = glsl.get_decoration(resource.id, spv::DecorationDescriptorSet);
            VulkShaderUBOBinding binding = (VulkShaderUBOBinding)glsl.get_decoration(resource.id, spv::DecorationBinding);
            VULK_ASSERT(*EnumNameVulkShaderUBOBinding(binding), "Invalid UBO binding " + std::to_string(binding) + " in shader " + parsedShader.name);
            parsedShader.uboBindings[binding] = resource.name;
        }

        // For SBOs
        for (const spirv_cross::Resource& resource : resources.storage_buffers) {
            // unsigned set = glsl.get_decoration(resource.id, spv::DecorationDescriptorSet);
            VulkShaderSSBOBinding binding = (VulkShaderSSBOBinding)glsl.get_decoration(resource.id, spv::DecorationBinding);
            VULK_ASSERT(*EnumNameVulkShaderSSBOBinding(binding), "Invalid SBO binding " + std::to_string(binding) + " in shader " + parsedShader.name);
            parsedShader.sboBindings[binding] = resource.name;
        }

        // For Samplers
        for (const spirv_cross::Resource& resource : resources.sampled_images) {
            // unsigned set = glsl.get_decoration(resource.id, spv::DecorationDescriptorSet);
            VulkShaderTextureBinding binding = (VulkShaderTextureBinding)glsl.get_decoration(resource.id, spv::DecorationBinding);
            auto foo = EnumNameVulkShaderTextureBinding(binding);
            VULK_ASSERT(*foo, "Invalid sampler binding " + std::to_string(binding) + " in shader " + parsedShader.name);
            parsedShader.samplerBindings[binding] = resource.name;
        }

        // Inputs
        for (const spirv_cross::Resource& resource : resources.stage_inputs) {
            VulkShaderLocation location = (VulkShaderLocation)glsl.get_decoration(resource.id, spv::DecorationLocation);
            VULK_ASSERT(*EnumNameVulkShaderLocation(location), "Invalid vertex input location " + std::to_string(location));
            parsedShader.inputLocations[location] = resource.name;
        }

        // Outputs
        for (const spirv_cross::Resource& resource : resources.stage_outputs) {
            VulkShaderLocation location = (VulkShaderLocation)glsl.get_decoration(resource.id, spv::DecorationLocation);
            VULK_ASSERT(*EnumNameVulkShaderLocation(location), "Invalid vertex input location " + std::to_string(location));
            parsedShader.outputLocations[location] = resource.name;
        }

        // Push constants
        for (const spirv_cross::Resource& resource : resources.push_constant_buffers) {
            // Get the buffer size
            spirv_cross::SPIRType const& type = glsl.get_type(resource.base_type_id);
            size_t buffer_size = glsl.get_declared_struct_size(type);
            // Note: Push constants do not have a traditional binding location like uniforms yet.
            unsigned location = 0;
            if (glsl.has_decoration(resource.id, spv::DecorationLocation)) {
                location = glsl.get_decoration(resource.id, spv::DecorationLocation);
            }
            logger()->trace("Push constant buffer: name={}, size={}, location={}", resource.name, buffer_size, location);
            VULK_ASSERT(location < 32, "Push constant location is too large"); // no idea what the max may eventually be but this isn't crazy
            if (parsedShader.pushConstants.size() <= location) {
                parsedShader.pushConstants.resize(location + 1);
            }
            parsedShader.pushConstants[location] |= shaderStage;
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

    // e.g. the "vert" or "frag" part of the descriptor set
    static void updateDSDef(ShaderInfo info, std::string stage, DescriptorSetDef& def) {
        VkShaderStageFlagBits stageFlag = DescriptorSetDef::getShaderStageFromStr(stage);
        for (auto& ubo : info.uboBindings) {
            def.uniformBuffers[stageFlag].push_back(ubo.first);
        }
        for (auto& sbo : info.sboBindings) {
            def.storageBuffers[stageFlag].push_back(sbo.first);
        }
        for (auto& sampler : info.samplerBindings) {
            def.imageSamplers[stageFlag].push_back(sampler.first);
        }
    }

    static BuiltPipelineDef buildPipeline(SourcePipelineDef pipelineIn, std::filesystem::path builtShadersDir) {
        if (!std::filesystem::exists(builtShadersDir)) {
            std::cerr << "Shaders directory does not exist: " << builtShadersDir << std::endl;
            VULK_THROW("PipelineBuilder: Shaders directory does not exist");
        }

        BuiltPipelineDef pipelineOut(pipelineIn);

        std::vector<ShaderInfo> shaderInfos;
        ShaderInfo vertShaderInfo = infoFromShader(pipelineIn.vertShaderName, "vert", builtShadersDir);
        shaderInfos.push_back(vertShaderInfo);
        updateDSDef(vertShaderInfo, "vert", pipelineOut.descriptorSet);

        if (!pipelineIn.geomShaderName.empty()) {
            ShaderInfo info = infoFromShader(pipelineIn.geomShaderName, "geom", builtShadersDir);
            shaderInfos.push_back(info);
            updateDSDef(info, "geom", pipelineOut.descriptorSet);
        }
        if (!pipelineIn.fragShaderName.empty()) {
            ShaderInfo info = infoFromShader(pipelineIn.fragShaderName, "frag", builtShadersDir);
            shaderInfos.push_back(info);
            updateDSDef(info, "frag", pipelineOut.descriptorSet);
        }

        for (auto& [location, name] : vertShaderInfo.inputLocations) {
            pipelineOut.vertInputs.push_back(location);
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

    static void buildPipelineFile(SourcePipelineDef pipelineIn, std::filesystem::path builtShadersDir, std::filesystem::path pipelineFileOut) {
        if (!std::filesystem::exists(builtShadersDir)) {
            std::cerr << "Shaders directory does not exist: " << builtShadersDir << std::endl;
            VULK_THROW("PipelineBuilder: Shaders directory does not exist");
        }
        if (!std::filesystem::exists(pipelineFileOut.parent_path())) {
            std::cerr << "Output directory does not exist: " << pipelineFileOut.parent_path() << std::endl;
            VULK_THROW("PipelineBuilder: Output directory does not exist");
        }

        BuiltPipelineDef pipelineOut = buildPipeline(pipelineIn, builtShadersDir);
        VulkCereal::inst()->toFile(pipelineFileOut, pipelineOut);
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

        nlohmann::json pipelineInJSON;
        std::ifstream inFile(pipelineFileSrc);
        inFile >> pipelineInJSON;
        inFile.close();
        SourcePipelineDef def = SourcePipelineDef::fromJSON(pipelineInJSON);

        buildPipelineFile(def, builtShadersDir, pipelineFileOut);
    }

    // static void buildPipelinesFromMetadata(std::filesystem::path builtShadersDir, std::filesystem::path pipelineFileOut, std::string optPipeline = "") {
    //     if (!std::filesystem::exists(builtShadersDir)) {
    //         std::cerr << "Shaders directory does not exist: " << builtShadersDir << std::endl;
    //         VULK_THROW("PipelineBuilder: Shaders directory does not exist");
    //     }
    //     if (!std::filesystem::exists(pipelineFileOut)) {
    //         std::cerr << "Output directory does not exist: " << pipelineFileOut << std::endl;
    //         VULK_THROW("PipelineBuilder: Output directory does not exist");
    //     }

    //     Metadata m = {};
    //     findAndProcessMetadata(builtShadersDir.parent_path(), m);
    //     std::string errMsg;
    //     if (optPipeline.size() > 0) {
    //         buildPipelineFile(*m.pipelines.at(optPipeline), builtShadersDir, pipelineFileOut / (optPipeline + ".json"), errMsg);
    //     } else {
    //         for (auto &pipeline : m.pipelines) {
    //             PipelineDeclDef def = *pipeline.second;
    //             buildPipelineFile(def, builtShadersDir, pipelineFileOut / (pipeline.first + ".json"), errMsg);
    //         }
    //     }
    //     if (!errMsg.empty()) {
    //         std::cerr << "Errors building pipelines: \n" << errMsg << std::endl;
    //         VULK_THROW("PipelineBuilder: Errors building pipelines");
    //     }
    // }

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