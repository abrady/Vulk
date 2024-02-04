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

// load resources used for rendering a set of things: shaders, meshes, textures, materials, etc.
// Note:
//  not thread safe,
//  not currently using reference counting so materials load twice
class VulkResources
{
public:
    Vulk &vk;

    struct XformsUBO
    {
        alignas(16) glm::mat4 world;
        alignas(16) glm::mat4 view;
        alignas(16) glm::mat4 proj;
    };

    struct ModelUBOs
    {
        VulkFrameUBOs<XformsUBO> xforms;
        VulkFrameUBOs<glm::vec3> eyePos;
        VulkFrameUBOs<VulkLight> lights;
        ModelUBOs(Vulk &vk) : xforms(vk), eyePos(vk), lights(vk) {}
    };
    ModelUBOs modelUBOs;

private:
    std::unordered_map<std::string, std::shared_ptr<VulkMaterialTextures>> materialTextures;
    std::unordered_map<std::string, std::shared_ptr<VulkUniformBuffer<VulkMaterialConstants>>> materialUBOs;
    std::unordered_map<std::string, std::shared_ptr<VulkMesh>> meshes;
    std::unordered_map<std::string, std::shared_ptr<VulkTextureView>> textureViews;
    std::unordered_map<std::string, std::shared_ptr<VulkTextureView>> normalViews;
    std::unordered_map<std::string, std::shared_ptr<VulkPipeline>> pipelines;
    std::unordered_map<std::string, std::shared_ptr<VulkDescriptorSetLayout>> descriptorSetLayouts;
    std::unordered_map<std::string, std::shared_ptr<VulkBuffer>> buffers;
    std::unordered_map<std::string, std::shared_ptr<VulkModel>> models;
    std::unordered_map<std::string, VkShaderModule> vertShaders, fragShaders;
    VkSampler textureSampler;

    enum ShaderType
    {
        Vertex,
        Geometry,
        Fragment
    };

    VkShaderModule createShaderModule(ShaderType type, std::string const &name);
    void loadMetadata();

    void loadDescriptorSetLayouts();
    void loadPipeline(std::string name);

    std::shared_ptr<VulkUniformBuffer<VulkMaterialConstants>> getMaterial(std::string const &name);
    std::shared_ptr<VulkMesh> getMesh(std::string const &name);
    std::shared_ptr<VulkMaterialTextures> getMaterialTextures(std::string const &name);

public:
    VulkResources(Vulk &vk)
        : vk(vk), modelUBOs(vk)
    {
        loadMetadata();
        textureSampler = vk.createTextureSampler();
        loadDescriptorSetLayouts();
    }

    VulkResources &loadActor(std::string name); // load from the metadata for rendering
    VulkResources &loadVertexShader(std::string name);
    VulkResources &loadFragmentShader(std::string name);

    VkShaderModule getVertexShader(std::string const &name);
    VkShaderModule getFragmentShader(std::string const &name);
    VkSampler getTextureSampler();
    VkPipeline getPipeline(std::string const &name);
    VkPipelineLayout getPipelineLayout(std::string const &name);
    std::shared_ptr<VulkModel> getModel(std::string const &name);

    ~VulkResources();
};