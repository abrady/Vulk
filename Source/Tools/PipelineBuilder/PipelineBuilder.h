#pragma once

#include "spirv_cross/spirv_glsl.hpp"
#include <filesystem>
#include <fstream>
#include <iostream>
#include <unordered_map>
#include <vector>

#include "Vulk/VulkResourceMetadata.h"
#include "Vulk/VulkShaderEnums.h"

#include <nlohmann/json.hpp>

struct ShaderInfo {
    std::string name;
    std::string entryPoint;
    std::unordered_map<VulkShaderUBOBindings, std::string> uboBindings;
    std::unordered_map<VulkShaderSSBOBindings, std::string> sboBindings;
    std::unordered_map<VulkShaderTextureBindings, std::string> samplerBindings;
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
            parsedShader.uboBindings[(VulkShaderUBOBindings)binding] = resource.name;
        }

        // For SBOs
        for (const spirv_cross::Resource &resource : resources.storage_buffers) {
            unsigned set = glsl.get_decoration(resource.id, spv::DecorationDescriptorSet);
            unsigned binding = glsl.get_decoration(resource.id, spv::DecorationBinding);
            parsedShader.sboBindings[(VulkShaderSSBOBindings)binding] = resource.name;
        }

        // For Samplers
        for (const spirv_cross::Resource &resource : resources.sampled_images) {
            unsigned set = glsl.get_decoration(resource.id, spv::DecorationDescriptorSet);
            unsigned binding = glsl.get_decoration(resource.id, spv::DecorationBinding);
            parsedShader.samplerBindings[(VulkShaderTextureBindings)binding] = resource.name;
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

    static ShaderInfo infoFromDef(ShaderDef def, std::string type, std::filesystem::path shadersDir) {
        return getShaderInfo(shadersDir / type / (def.name + "." + type + "spv"));
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

    static PipelineDef buildPipeline(PipelineDef pipelineIn, std::filesystem::path builtShadersDir) {
        if (!std::filesystem::exists(builtShadersDir)) {
            std::cerr << "Shaders directory does not exist: " << builtShadersDir << std::endl;
            throw std::runtime_error("PipelineBuilder: Shaders directory does not exist");
        }

        PipelineDef pipelineOut = pipelineIn;

        std::vector<ShaderInfo> shaderInfos;
        if (pipelineIn.vertexShader) {
            ShaderInfo info = infoFromDef(*pipelineIn.vertexShader, "vert", builtShadersDir);
            shaderInfos.push_back(info);
            updateDSDef(info, "vert", pipelineOut.descriptorSet);
        }
        if (pipelineIn.geometryShader) {
            ShaderInfo info = infoFromDef(*pipelineIn.geometryShader, "geom", builtShadersDir);
            shaderInfos.push_back(info);
            updateDSDef(info, "geom", pipelineOut.descriptorSet);
        }
        if (pipelineIn.fragmentShader) {
            ShaderInfo info = infoFromDef(*pipelineIn.fragmentShader, "frag", builtShadersDir);
            shaderInfos.push_back(info);
            updateDSDef(info, "frag", pipelineOut.descriptorSet);
        }

        for (int i = 0; i < shaderInfos.size() - 1; i++) {
            std::string errMsg;
            if (!checkConnections(shaderInfos[i], shaderInfos[i + 1], errMsg)) {
                std::cerr << errMsg << std::endl;
                throw std::runtime_error("PipelineBuilder: Shader connections do not match");
            }
        }

        return pipelineOut;
    }

    static void buildPipelineFile(PipelineDef pipelineIn, std::filesystem::path builtShadersDir, std::filesystem::path pipelineFileOut) {
        if (!std::filesystem::exists(builtShadersDir)) {
            std::cerr << "Shaders directory does not exist: " << builtShadersDir << std::endl;
            throw std::runtime_error("PipelineBuilder: Shaders directory does not exist");
        }
        if (!std::filesystem::exists(pipelineFileOut.parent_path())) {
            std::cerr << "Output directory does not exist: " << pipelineFileOut.parent_path() << std::endl;
            throw std::runtime_error("PipelineBuilder: Output directory does not exist");
        }

        PipelineDef pipelineOut = buildPipeline(pipelineIn, builtShadersDir);
        nlohmann::json pipelineOutJSON = PipelineDef::toJSON(pipelineOut);

        std::ofstream outFile(pipelineFileOut);
        outFile << pipelineOutJSON;
        outFile.close();
    }

    static void buildPipelinesFromMetadata(std::filesystem::path builtShadersDir, std::filesystem::path pipelineDirOut) {
        if (!std::filesystem::exists(builtShadersDir)) {
            std::cerr << "Shaders directory does not exist: " << builtShadersDir << std::endl;
            throw std::runtime_error("PipelineBuilder: Shaders directory does not exist");
        }
        if (!std::filesystem::exists(pipelineDirOut)) {
            std::cerr << "Output directory does not exist: " << pipelineDirOut << std::endl;
            throw std::runtime_error("PipelineBuilder: Output directory does not exist");
        }

        Metadata const *m = getMetadata();
        for (auto &pipeline : m->pipelines) {
            PipelineDef def = *pipeline.second;
            buildPipelineFile(def, builtShadersDir, pipelineDirOut / (pipeline.first + ".json"));
        }
    }

  private:
    static std::vector<uint32_t> readSPIRVFile(std::filesystem::path filename) {
        std::ifstream file(filename, std::ios::ate | std::ios::binary);

        if (!file.is_open()) {
            throw std::runtime_error("Failed to open file!");
        }

        size_t fileSize = (size_t)file.tellg();
        std::vector<uint32_t> buffer(fileSize / sizeof(uint32_t));

        file.seekg(0);
        file.read((char *)buffer.data(), fileSize);
        file.close();

        return buffer;
    }
};