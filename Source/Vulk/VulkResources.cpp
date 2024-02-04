#include "VulkResources.h"

#include <unordered_map>
#include <iostream>
#include <filesystem>
#include <string>
#include <nlohmann/json.hpp>

#include "VulkMesh.h"
#include "VulkPipelineBuilder.h"
#include "VulkDescriptorSetLayoutBuilder.h"
#include "VulkDescriptorSetBuilder.h"
#include "private/VulkResourceMetadata.h"

using namespace std;
namespace fs = filesystem;

void VulkResources::loadMetadata()
{
    static once_flag flag;
    call_once(flag, [&]()
              { findAndProcessMetadata(fs::current_path()); });
}

VkShaderModule VulkResources::createShaderModule(ShaderType type, string const &name)
{
    fs::path subdir;
    char const *suffix;
    switch (type)
    {
    case Vertex:
        subdir = "Vert";
        suffix = ".vertspv";
        break;
    case Geometry:
        subdir = "Geom";
        suffix = ".geomspv";
        break;
    case Fragment:
        subdir = "Frag";
        suffix = ".fragspv";
        break;
    default:
        throw runtime_error("Invalid shader type");
    };

    fs::path path = fs::current_path() / "Source" / "Shaders" / subdir / (name + suffix);
    auto shaderCode = readFileIntoMem(path.string());
    VkShaderModule shaderModule = vk.createShaderModule(shaderCode);
    return shaderModule;
}

shared_ptr<VulkModel> VulkResources::getModel(string const &name)
{
    if (!models.contains(name))
    {
        ModelDef &modelDef = *metadata.models.at(name);
        shared_ptr<VulkMaterialTextures> textures = getMaterialTextures(modelDef.material->name);
        auto mr = make_shared<VulkModel>(vk, getMesh(modelDef.mesh->name), textures, getMaterial(modelDef.material->name));
        mr->setDescriptorSets(VulkDescriptorSetBuilder(vk)
                                  .addUniformBuffers(modelUBOs.xforms, VK_SHADER_STAGE_VERTEX_BIT, VulkShaderUBOBinding_Xforms)
                                  .addUniformBuffers(modelUBOs.eyePos, VK_SHADER_STAGE_FRAGMENT_BIT, VulkShaderUBOBinding_EyePos)
                                  .addUniformBuffers(modelUBOs.lights, VK_SHADER_STAGE_FRAGMENT_BIT, VulkShaderUBOBinding_Lights)
                                  .addUniformBuffer(*materialUBOs.at(name), VK_SHADER_STAGE_FRAGMENT_BIT, VulkShaderUBOBinding_MaterialUBO)
                                  .addImageSampler(VK_SHADER_STAGE_FRAGMENT_BIT, VulkShaderTextureBinding_TextureSampler, textures->diffuseView->textureImageView, getTextureSampler())
                                  .addImageSampler(VK_SHADER_STAGE_FRAGMENT_BIT, VulkShaderTextureBinding_NormalSampler, textures->normalView->textureImageView, getTextureSampler())
                                  .build());
        models[name] = mr;
    }
    return models[name];
}

VulkResources &VulkResources::loadActor(string name)
{
    // if (actors.contains(name))
    // {
    //     return *this;
    // }
    ActorDef &actorDef = *metadata.actors.at(name);
    shared_ptr<VulkModel> model = getModel(actorDef.model->name);

    // actors[name] = mr;
    return *this;
}

VulkResources &VulkResources::loadVertexShader(string name)
{
    ASSERT_KEY_NOT_SET(vertShaders, name);
    VkShaderModule shaderModule = createShaderModule(Vertex, name);
    vertShaders[name] = shaderModule;
    return *this;
}

VulkResources &VulkResources::loadFragmentShader(string name)
{
    ASSERT_KEY_NOT_SET(fragShaders, name);
    VkShaderModule shaderModule = createShaderModule(Fragment, name);
    fragShaders[name] = shaderModule;
    return *this;
}

void VulkResources::loadDescriptorSetLayouts()
{
    descriptorSetLayouts["LitModel"] = VulkDescriptorSetLayoutBuilder(vk)
                                           .addUniformBuffer(VK_SHADER_STAGE_VERTEX_BIT, VulkShaderUBOBinding_Xforms)
                                           .addUniformBuffer(VK_SHADER_STAGE_FRAGMENT_BIT, VulkShaderUBOBinding_EyePos)
                                           .addUniformBuffer(VK_SHADER_STAGE_FRAGMENT_BIT, VulkShaderUBOBinding_Lights)
                                           .addUniformBuffer(VK_SHADER_STAGE_FRAGMENT_BIT, VulkShaderUBOBinding_MaterialUBO)
                                           .addImageSampler(VK_SHADER_STAGE_FRAGMENT_BIT, VulkShaderTextureBinding_TextureSampler)
                                           .addImageSampler(VK_SHADER_STAGE_FRAGMENT_BIT, VulkShaderTextureBinding_NormalSampler)
                                           .build();
}

void VulkResources::loadPipeline(string name)
{
    if (pipelines.find(name) != pipelines.end())
    {
        return;
    }
    PipelineDef const &def = *metadata.pipelines.at(name);
    VulkDescriptorSetLayoutBuilder dslb(vk);

    for (auto &pair : def.descriptorSet.uniformBuffers)
        dslb.addUniformBuffer(pair.second, pair.first);
    for (auto &pair : def.descriptorSet.storageBuffers)
        dslb.addStorageBuffer(pair.second, pair.first);
    for (auto &pair : def.descriptorSet.imageSamplers)
        dslb.addImageSampler(pair.second, pair.first);

    pipelines[name] = VulkPipelineBuilder(vk)
                          .addVertexShaderStage(getVertexShader(def.vertexShader->name))
                          .addFragmentShaderStage(getFragmentShader(def.fragmentShader->name))
                          .addVulkVertexInput(def.vertexInputBinding)
                          .setPrimitiveTopology(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST)
                          .setLineWidth(1.0f)
                          .setCullMode(VK_CULL_MODE_BACK_BIT)
                          .setDepthTestEnabled(true)
                          .setDepthWriteEnabled(true)
                          .setDepthCompareOp(VK_COMPARE_OP_LESS)
                          .setStencilTestEnabled(false)
                          .setBlendingEnabled(true)
                          .build(descriptorSetLayouts.at(def.name)->layout);
}

shared_ptr<VulkUniformBuffer<VulkMaterialConstants>> VulkResources::getMaterial(string const &name)
{
    if (!materialUBOs.contains(name))
    {
        materialUBOs[name] = make_shared<VulkUniformBuffer<VulkMaterialConstants>>(vk, metadata.materials.at(name)->toVulkMaterialConstants());
    }
    return materialUBOs[name];
}

shared_ptr<VulkMesh> VulkResources::getMesh(string const &name)
{
    if (!meshes.contains(name))
    {
        meshes[name] = make_shared<VulkMesh>(VulkMesh::loadFromPath(metadata.meshes.at(name)->path));
    }
    return meshes[name];
}

shared_ptr<VulkMaterialTextures> VulkResources::getMaterialTextures(string const &name)
{
    if (!materialTextures.contains(name))
    {
        MaterialDef const &def = *metadata.materials.at(name);
        materialTextures[name] = make_shared<VulkMaterialTextures>();
        materialTextures[name]->diffuseView = make_unique<VulkTextureView>(vk, def.mapKd);
        materialTextures[name]->normalView = make_unique<VulkTextureView>(vk, def.mapNormal);
    }
    return materialTextures[name];
}

VkShaderModule VulkResources::getVertexShader(string const &name)
{
    if (vertShaders.find(name) == vertShaders.end())
    {
        loadVertexShader(name);
    }
    ASSERT_KEY_SET(vertShaders, name);
    return vertShaders[name];
}

VkShaderModule VulkResources::getFragmentShader(string const &name)
{
    if (fragShaders.find(name) == fragShaders.end())
    {
        loadFragmentShader(name);
    }
    ASSERT_KEY_SET(fragShaders, name);
    return fragShaders[name];
}

VkSampler VulkResources::getTextureSampler()
{
    return textureSampler;
}

VkPipeline VulkResources::getPipeline(string const &name)
{
    loadPipeline(name);
    VulkPipeline *p = pipelines.at(name).get();
    assert(p->pipeline != VK_NULL_HANDLE);
    return p->pipeline;
}

VkPipelineLayout VulkResources::getPipelineLayout(string const &name)
{
    loadPipeline(name);
    VulkPipeline *p = pipelines.at(name).get();
    assert(p->pipelineLayout != VK_NULL_HANDLE);
    return p->pipelineLayout;
}

VulkResources::~VulkResources()
{
    for (auto &pair : vertShaders)
    {
        vkDestroyShaderModule(vk.device, pair.second, nullptr);
    }
    for (auto &pair : fragShaders)
    {
        vkDestroyShaderModule(vk.device, pair.second, nullptr);
    }
    vkDestroySampler(vk.device, textureSampler, nullptr);
}
