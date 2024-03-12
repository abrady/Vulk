#pragma once

#include "spirv_cross/spirv_glsl.hpp"
#include <filesystem>
#include <fstream>
#include <iostream>
#include <unordered_map>
#include <vector>

#include "Vulk/VulkResourceMetadata.h"

#include <nlohmann/json.hpp>

struct ShaderInfo {
    std::string name;
    std::string entryPoint;
    std::unordered_map<VulkShaderUBOBinding, std::string> uboBindings;
    std::unordered_map<VulkShaderSSBOBinding, std::string> sboBindings;
    std::unordered_map<VulkShaderTextureBinding, std::string> samplerBindings;
    std::unordered_map<uint32_t, std::string> inputLocations;
    std::unordered_map<uint32_t, std::string> outputLocations;
};

class PipelineBuilder {
  public:
    static ShaderInfo getShaderInfo(std::filesystem::path shaderPath) {
        auto spirvData = readSPIRVFile(shaderPath);
        spirv_cross::CompilerGLSL glsl(std::move(spirvData));
        const spirv_cross::ShaderResources resources = glsl.get_shader_resources();

        ShaderInfo parsedShader;
        parsedShader.name = shaderPath.stem().string();
        parsedShader.entryPoint = glsl.get_entry_points_and_stages()[0].name;

        // For UBOs
        for (const spirv_cross::Resource &resource : resources.uniform_buffers) {
            unsigned set = glsl.get_decoration(resource.id, spv::DecorationDescriptorSet);
            unsigned binding = glsl.get_decoration(resource.id, spv::DecorationBinding);
            parsedShader.uboBindings[(VulkShaderUBOBinding)binding] = resource.name;
        }

        // For SBOs
        for (const spirv_cross::Resource &resource : resources.storage_buffers) {
            unsigned set = glsl.get_decoration(resource.id, spv::DecorationDescriptorSet);
            unsigned binding = glsl.get_decoration(resource.id, spv::DecorationBinding);
            parsedShader.sboBindings[(VulkShaderSSBOBinding)binding] = resource.name;
        }

        // For Samplers
        for (const spirv_cross::Resource &resource : resources.sampled_images) {
            unsigned set = glsl.get_decoration(resource.id, spv::DecorationDescriptorSet);
            unsigned binding = glsl.get_decoration(resource.id, spv::DecorationBinding);
            parsedShader.samplerBindings[(VulkShaderTextureBinding)binding] = resource.name;
        }

        // Inputs
        for (const spirv_cross::Resource &resource : resources.stage_inputs) {
            unsigned location = glsl.get_decoration(resource.id, spv::DecorationLocation);
            parsedShader.inputLocations[location] = resource.name;
        }

        // Outputs
        for (const spirv_cross::Resource &resource : resources.stage_outputs) {
            unsigned location = glsl.get_decoration(resource.id, spv::DecorationLocation);
            parsedShader.outputLocations[location] = resource.name;
        }

        return parsedShader;
    }

    static bool checkConnections(ShaderInfo &upstream, ShaderInfo &downstream, std::string &errMsg) {
        errMsg = "";
        // Check if the downstream shader has any inputs that are not outputs of the upstream shader
        for (auto &input : downstream.inputLocations) {
            if (upstream.outputLocations.find(input.first) == upstream.outputLocations.end()) {
                errMsg += "Downstream shader " + downstream.name + " has input " + input.second + " that is not an output of the upstream shader " +
                          upstream.name + "\n";
            }
        }
        for (auto &output : upstream.outputLocations) {
            if (downstream.inputLocations.find(output.first) == downstream.inputLocations.end()) {
                errMsg += "Upstream shader " + upstream.name + " has output " + output.second + " that is not an input of the downstream shader " +
                          downstream.name + "\n";
            }
        }
        return errMsg.empty();
    }

    static ShaderInfo infoFromShader(std::string name, std::string type, std::filesystem::path shadersDir) {
        return getShaderInfo(shadersDir / type / (name + "." + type + "spv"));
    }

    // e.g. the "vert" or "frag" part of the descriptor set
    static void updateDSDef(ShaderInfo info, std::string stage, DescriptorSetDef &def) {
        VkShaderStageFlagBits stageFlag = DescriptorSetDef::getShaderStageFromStr(stage);
        for (auto &ubo : info.uboBindings) {
            def.uniformBuffers[stageFlag].push_back(ubo.first);
        }
        for (auto &sbo : info.sboBindings) {
            def.storageBuffers[stageFlag].push_back(sbo.first);
        }
        for (auto &sampler : info.samplerBindings) {
            def.imageSamplers[stageFlag].push_back(sampler.first);
        }
    }

    static PipelineDeclDef buildPipeline(PipelineDeclDef pipelineIn, std::filesystem::path builtShadersDir) {
        if (!std::filesystem::exists(builtShadersDir)) {
            std::cerr << "Shaders directory does not exist: " << builtShadersDir << std::endl;
            VULK_THROW("PipelineBuilder: Shaders directory does not exist");
        }

        PipelineDeclDef pipelineOut = pipelineIn;

        std::vector<ShaderInfo> shaderInfos;
        if (!pipelineIn.vertShaderName.empty()) {
            ShaderInfo info = infoFromShader(pipelineIn.vertShaderName, "vert", builtShadersDir);
            shaderInfos.push_back(info);
            updateDSDef(info, "vert", pipelineOut.descriptorSet);
        }
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

        std::string errMsgOut;
        for (int i = 0; i < shaderInfos.size() - 1; i++) {
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

    static void buildPipelineFile(PipelineDeclDef pipelineIn, std::filesystem::path builtShadersDir, std::filesystem::path pipelineFileOut) {
        if (!std::filesystem::exists(builtShadersDir)) {
            std::cerr << "Shaders directory does not exist: " << builtShadersDir << std::endl;
            VULK_THROW("PipelineBuilder: Shaders directory does not exist");
        }
        if (!std::filesystem::exists(pipelineFileOut.parent_path())) {
            std::cerr << "Output directory does not exist: " << pipelineFileOut.parent_path() << std::endl;
            VULK_THROW("PipelineBuilder: Output directory does not exist");
        }

        PipelineDeclDef pipelineOut = buildPipeline(pipelineIn, builtShadersDir);
        nlohmann::json pipelineOutJSON = PipelineDeclDef::toJSON(pipelineOut);

        std::ofstream outFile(pipelineFileOut);
        outFile << pipelineOutJSON.dump(4);
        outFile.close();
    }

    static void buildPipelineFromFile(std::filesystem::path builtShadersDir, std::filesystem::path pipelineDirOut, fs::path pipelineFileIn) {
        if (!std::filesystem::exists(pipelineFileIn)) {
            std::cerr << "Pipeline file does not exist: " << pipelineFileIn << std::endl;
            VULK_THROW("PipelineBuilder: Pipeline file does not exist");
        }
        if (!std::filesystem::exists(builtShadersDir)) {
            std::cerr << "Shaders directory does not exist: " << builtShadersDir << std::endl;
            VULK_THROW("PipelineBuilder: Shaders directory does not exist");
        }
        if (!std::filesystem::exists(pipelineDirOut)) {
            std::cerr << "Output directory does not exist: " << pipelineDirOut << std::endl;
            VULK_THROW("PipelineBuilder: Output directory does not exist");
        }

        nlohmann::json pipelineInJSON;
        std::ifstream inFile(pipelineFileIn);
        inFile >> pipelineInJSON;
        inFile.close();
        PipelineDeclDef def = PipelineDeclDef::fromJSON(pipelineInJSON);

        buildPipelineFile(def, builtShadersDir, pipelineDirOut / pipelineFileIn.filename());
    }

    // static void buildPipelinesFromMetadata(std::filesystem::path builtShadersDir, std::filesystem::path pipelineDirOut, std::string optPipeline = "") {
    //     if (!std::filesystem::exists(builtShadersDir)) {
    //         std::cerr << "Shaders directory does not exist: " << builtShadersDir << std::endl;
    //         VULK_THROW("PipelineBuilder: Shaders directory does not exist");
    //     }
    //     if (!std::filesystem::exists(pipelineDirOut)) {
    //         std::cerr << "Output directory does not exist: " << pipelineDirOut << std::endl;
    //         VULK_THROW("PipelineBuilder: Output directory does not exist");
    //     }

    //     Metadata m = {};
    //     findAndProcessMetadata(builtShadersDir.parent_path(), m);
    //     std::string errMsg;
    //     if (optPipeline.size() > 0) {
    //         buildPipelineFile(*m.pipelines.at(optPipeline), builtShadersDir, pipelineDirOut / (optPipeline + ".json"), errMsg);
    //     } else {
    //         for (auto &pipeline : m.pipelines) {
    //             PipelineDeclDef def = *pipeline.second;
    //             buildPipelineFile(def, builtShadersDir, pipelineDirOut / (pipeline.first + ".json"), errMsg);
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
        file.read((char *)buffer.data(), fileSize);
        file.close();

        return buffer;
    }
};