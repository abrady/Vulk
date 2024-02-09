#pragma once

#include <unordered_map>
#include <iostream>
#include <filesystem>
#include <string>

#include "Vulk.h"
#include "VulkTextureView.h"
#include "VulkMesh.h"
#include "VulkDescriptorSetLayoutBuilder.h"
#include "VulkFrameUBOs.h"
#include "VulkBufferBuilder.h"
#include "VulkDescriptorSetBuilder.h"
#include "VulkPipelineBuilder.h"
#include "VulkModel.h"
#include "VulkMaterialTextures.h"
#include "VulkActor.h"
#include "VulkScene.h"
#include "VulkShaderModule.h"
#include "VulkSampler.h"

struct MeshDef;
struct ModelDef;
struct PipelineDef;

// load resources used for rendering a set of things: shaders, meshes, textures, materials, etc.
// Note:
//  not thread safe,
//  not currently using reference counting so materials load twice
class VulkResources
{
public:
    Vulk &vk;

private:
    enum ShaderType
    {
        Vertex,
        Geometry,
        Fragment
    };

    std::shared_ptr<VulkShaderModule> createShaderModule(ShaderType type, std::string const &name);
    void loadMetadata();

    std::shared_ptr<VulkPipeline> loadPipeline(PipelineDef &pipelineDef);

    std::shared_ptr<VulkUniformBuffer<VulkMaterialConstants>> getMaterial(std::string const &name);
    std::shared_ptr<VulkMesh> getMesh(MeshDef &meshDef);
    std::shared_ptr<VulkMaterialTextures> getMaterialTextures(std::string const &name);
    std::shared_ptr<VulkModel> getModel(ModelDef &modelDef);

public:
    std::unordered_map<std::string, std::shared_ptr<VulkMaterialTextures>> materialTextures;
    std::unordered_map<std::string, std::shared_ptr<VulkUniformBuffer<VulkMaterialConstants>>> materialUBOs;
    std::unordered_map<std::string, std::shared_ptr<VulkMesh>> meshes;
    std::unordered_map<std::string, std::shared_ptr<VulkPipeline>> pipelines;
    std::unordered_map<std::string, std::shared_ptr<VulkBuffer>> buffers;
    std::unordered_map<std::string, std::shared_ptr<VulkModel>> models;
    std::unordered_map<std::string, std::shared_ptr<VulkScene>> scenes;
    std::unordered_map<std::string, std::shared_ptr<VulkShaderModule>> vertShaders, geomShaders, fragShaders;
    std::shared_ptr<VulkSampler> textureSampler;

    VulkResources(Vulk &vk)
        : vk(vk)
    {
        loadMetadata();
        textureSampler = std::make_shared<VulkSampler>(vk);
    }

    VulkResources &loadScene(std::string name);

    std::shared_ptr<VulkShaderModule> getVertexShader(std::string const &name)
    {
        if (!vertShaders.contains(name))
            vertShaders[name] = createShaderModule(Vertex, name);
        return vertShaders.at(name);
    }
    std::shared_ptr<VulkShaderModule> getGeometryShader(std::string const &name)
    {
        if (!geomShaders.contains(name))
            geomShaders[name] = createShaderModule(Geometry, name);
        return geomShaders.at(name);
    }
    std::shared_ptr<VulkShaderModule> getFragmentShader(std::string const &name)
    {
        if (!fragShaders.contains(name))
            fragShaders[name] = createShaderModule(Fragment, name);
        return fragShaders.at(name);
    }

    VkPipeline getPipeline(std::string const &name);
};