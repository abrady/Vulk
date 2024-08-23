#pragma once

#include <filesystem>
#include <iostream>
#include <string>
#include <unordered_map>

#include "Vulk.h"
#include "VulkActor.h"
#include "VulkDescriptorSetLayoutBuilder.h"
#include "VulkFrameUBOs.h"
#include "VulkImageView.h"
#include "VulkMaterialTextures.h"
#include "VulkMesh.h"
#include "VulkModel.h"
#include "VulkPipeline.h"
#include "VulkSampler.h"
#include "VulkScene.h"
#include "VulkShaderModule.h"

struct ActorDef;
struct DescriptorSetDef;
struct MeshDef;
struct ModelDef;
struct PipelineDef;
struct Metadata;

// It is expected that an instance of this will be made for a world/level/scene, the approriate resources will be loaded
// off of it, and then this can be destructed at the end of the loading process to free up unused resources
// while used resources will be kept alive by the shared_ptrs that are returned to the caller.
//
// Note: not thread safe
class VulkResources {
   public:
    Vulk& vk;
    std::shared_ptr<Metadata const> metadata;

    VulkResources(Vulk& vk);
    VulkResources(Vulk& vk, std::shared_ptr<Metadata> metadata);
    static std::shared_ptr<VulkResources> loadFromProject(Vulk& vk, std::filesystem::path projectDir);

   private:
    enum ShaderType { Vert, Geom, Frag };

    std::shared_ptr<VulkShaderModule> createShaderModule(ShaderType type, std::string const& name);

    std::shared_ptr<VulkUniformBuffer<VulkMaterialConstants>> getMaterial(std::string const& name);
    std::shared_ptr<VulkMesh> getMesh(MeshDef& meshDef);
    std::shared_ptr<VulkMaterialTextures> getMaterialTextures(std::string const& name);
    std::shared_ptr<VulkModel> getModel(ModelDef const& modelDef, PipelineDef const& pipelineDef);

   public:
    std::unordered_map<std::string, std::shared_ptr<VulkMaterialTextures>> materialTextures;
    std::unordered_map<std::string, std::shared_ptr<VulkUniformBuffer<VulkMaterialConstants>>> materialUBOs;
    std::unordered_map<std::string, std::shared_ptr<VulkMesh>> meshes;
    std::unordered_map<std::string, std::shared_ptr<VulkPipeline>> pipelines;
    std::unordered_map<std::string, std::shared_ptr<VulkBuffer>> buffers;
    std::unordered_map<std::string, std::shared_ptr<VulkModel>> pipelineModels;
    std::unordered_map<std::string, std::shared_ptr<VulkScene>> scenes;
    std::unordered_map<std::string, std::shared_ptr<VulkShaderModule>> vertShaders, geomShaders, fragShaders;
    std::shared_ptr<VulkSampler> textureSampler, shadowMapSampler;

    std::shared_ptr<VulkScene>
    loadScene(std::string name, std::array<std::shared_ptr<VulkDepthView>, MAX_FRAMES_IN_FLIGHT> shadowMapViews);

    std::shared_ptr<VulkShaderModule> getvertShader(std::string const& name) {
        if (!vertShaders.contains(name))
            vertShaders[name] = createShaderModule(Vert, name);
        return vertShaders.at(name);
    }
    std::shared_ptr<VulkShaderModule> getGeometryShader(std::string const& name) {
        if (!geomShaders.contains(name))
            geomShaders[name] = createShaderModule(Geom, name);
        return geomShaders.at(name);
    }
    std::shared_ptr<VulkShaderModule> getFragmentShader(std::string const& name) {
        if (!fragShaders.contains(name))
            fragShaders[name] = createShaderModule(Frag, name);
        return fragShaders.at(name);
    }

    std::shared_ptr<VulkActor>
    createActorFromPipeline(ActorDef const& actorDef, std::shared_ptr<VulkPipeline> pipeline, std::shared_ptr<VulkScene> scene);

    std::shared_ptr<VulkDescriptorSetLayout> buildDescriptorSetLayoutFromPipeline(std::string name);
    std::shared_ptr<VulkPipeline> loadPipeline(VkRenderPass renderPass, VkExtent2D extent, std::string const& name);
    std::shared_ptr<VulkPipeline> getPipeline(std::string const& name) {
        return pipelines.at(name);
    }
};