#include "VulkResources.h"

#include <unordered_map>
#include <iostream>
#include <filesystem>
#include <string>
#include <nlohmann/json.hpp>

#include "VulkCamera.h"
#include "VulkPointLight.h"
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

std::shared_ptr<VulkShaderModule> VulkResources::createShaderModule(ShaderType type, string const &name)
{
    fs::path subdir;
    char const *suffix;
    std::unordered_map<std::string, std::shared_ptr<VulkShaderModule>> *shaders_map;
    switch (type)
    {
    case Vertex:
        subdir = "Vert";
        suffix = ".vertspv";
        shaders_map = &vertShaders;
        break;
    case Geometry:
        subdir = "Geom";
        suffix = ".geomspv";
        shaders_map = &geomShaders;
        break;
    case Fragment:
        subdir = "Frag";
        suffix = ".fragspv";
        shaders_map = &fragShaders;
        break;
    default:
        throw runtime_error("Invalid shader type");
    };

    fs::path path = fs::current_path() / "Source" / "Shaders" / subdir / (name + suffix);
    auto shaderCode = readFileIntoMem(path.string());
    VkShaderModule shaderModule = vk.createShaderModule(shaderCode);
    auto sm = make_shared<VulkShaderModule>(vk, shaderModule);
    shaders_map->insert({name, sm});
    return sm;
}

shared_ptr<VulkModel> VulkResources::getModel(ModelDef &modelDef)
{
    string name = modelDef.name;
    if (!models.contains(name))
    {
        shared_ptr<VulkMaterialTextures> textures = getMaterialTextures(modelDef.material->name);
        auto mr = make_shared<VulkModel>(vk, getMesh(*modelDef.mesh), textures, getMaterial(modelDef.material->name));
        models[name] = mr;
    }
    return models[name];
}

std::shared_ptr<VulkPipeline> VulkResources::loadPipeline(PipelineDef &def)
{
    string name = def.name;
    if (pipelines.contains(name))
    {
        return pipelines[name];
    }
    VulkDescriptorSetLayoutBuilder dslb(vk);

    for (auto &pair : def.descriptorSet.uniformBuffers)
        dslb.addUniformBuffer(pair.second, pair.first);
    for (auto &pair : def.descriptorSet.storageBuffers)
        dslb.addStorageBuffer(pair.second, pair.first);
    for (auto &pair : def.descriptorSet.imageSamplers)
        dslb.addImageSampler(pair.second, pair.first);

    shared_ptr<VulkDescriptorSetLayout> descriptorSetLayout = dslb.build();

    auto p = VulkPipelineBuilder(vk)
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
                 .build(descriptorSetLayout);
    pipelines[name] = p;
    return p;
}

VulkResources &VulkResources::loadScene(std::string name)
{
    if (scenes.contains(name))
    {
        return *this;
    }
    SceneDef &sceneDef = *metadata.scenes.at(name);
    shared_ptr<VulkScene> scene = make_shared<VulkScene>(vk);

    scene->camera = sceneDef.camera;
    *scene->sceneUBOs.pointLight.mappedUBO = *sceneDef.pointLights[0]; // just one light for now

    for (auto &actorDef : sceneDef.actors)
    {
        shared_ptr<VulkModel> model = getModel(*actorDef->model);
        shared_ptr<VulkPipeline> pipeline = loadPipeline(*actorDef->pipeline);

        // Create a descriptor set for the actor that matches the pipeline layout
        DescriptorSetDef const &dsDef = metadata.pipelines.at(actorDef->pipeline->name)->descriptorSet;
        auto const &ubs = dsDef.uniformBuffers;
        auto binding = dsDef.uniformBuffers.begin(); // just to get the type
        VulkDescriptorSetBuilder builder(vk);
        if ((binding = ubs.find(VulkShaderUBOBinding_Xforms)) != ubs.end())
            builder.addFrameUBOs(scene->sceneUBOs.xforms, binding->second, binding->first);
        if ((binding = ubs.find(VulkShaderUBOBinding_MaterialUBO)) != ubs.end())
            builder.addUniformBuffer(*model->materialUBO, binding->second, binding->first);
        if ((binding = ubs.find(VulkShaderUBOBinding_Lights)) != ubs.end())
            builder.addUniformBuffer(scene->sceneUBOs.pointLight, binding->second, binding->first);
        if ((binding = ubs.find(VulkShaderUBOBinding_EyePos)) != ubs.end())
            builder.addFrameUBOs(scene->sceneUBOs.eyePos, binding->second, binding->first);
        if (dsDef.imageSamplers.contains(VulkShaderTextureBinding_TextureSampler))
            builder.addImageSampler(dsDef.imageSamplers.at(VulkShaderTextureBinding_TextureSampler), VulkShaderTextureBinding_TextureSampler, model->textures->diffuseView, textureSampler);
        if (dsDef.imageSamplers.contains(VulkShaderTextureBinding_NormalSampler))
            builder.addImageSampler(dsDef.imageSamplers.at(VulkShaderTextureBinding_NormalSampler), VulkShaderTextureBinding_NormalSampler, model->textures->normalView, textureSampler);

        scene->actors.push_back(make_shared<VulkActor>(vk, model, actorDef->xform, pipeline, builder));
    }
    scenes[name] = scene;
    return *this;
}

shared_ptr<VulkUniformBuffer<VulkMaterialConstants>> VulkResources::getMaterial(string const &name)
{
    if (!materialUBOs.contains(name))
    {
        materialUBOs[name] = make_shared<VulkUniformBuffer<VulkMaterialConstants>>(vk, metadata.materials.at(name)->toVulkMaterialConstants());
    }
    return materialUBOs[name];
}

shared_ptr<VulkMesh> VulkResources::getMesh(MeshDef &meshDef)
{
    string name = meshDef.name;
    if (meshes.contains(name))
    {
        return meshes[name];
    }
    shared_ptr<VulkMesh> m;
    switch (meshDef.type)
    {
    case MeshDefType_Model:
        m = make_shared<VulkMesh>(VulkMesh::loadFromPath(meshDef.getModel()->path, name));
        break;
    case MeshDefType_GeoMesh:
    {
        GeoMeshDef &geoMeshDef = *meshDef.getGeoMesh();
        m = make_shared<VulkMesh>();
        m->name = name;
        switch (geoMeshDef.type)
        {
        case GeoMeshDefType_Sphere:
            makeGeoSphere(geoMeshDef.sphere.radius, geoMeshDef.sphere.numSubdivisions, *m);
            break;
        case GeoMeshDefType_Cylinder:
            makeCylinder(geoMeshDef.cylinder.height, geoMeshDef.cylinder.bottomRadius, geoMeshDef.cylinder.topRadius, geoMeshDef.cylinder.numStacks, geoMeshDef.cylinder.numSlices, *m);
            break;
        }
    }
    break;
    default:
        throw runtime_error("Invalid mesh type");
    }
    meshes[name] = m;
    return m;
}

shared_ptr<VulkMaterialTextures> VulkResources::getMaterialTextures(string const &name)
{
    if (!materialTextures.contains(name))
    {
        MaterialDef const &def = *metadata.materials.at(name);
        materialTextures[name] = make_shared<VulkMaterialTextures>();
        materialTextures[name]->diffuseView = !def.mapKd.empty() ? make_unique<VulkTextureView>(vk, def.mapKd) : nullptr;
        materialTextures[name]->normalView = !def.mapNormal.empty() ? make_unique<VulkTextureView>(vk, def.mapNormal) : nullptr;
    }
    return materialTextures[name];
}

VkPipeline VulkResources::getPipeline(string const &name)
{
    VulkPipeline *p = pipelines.at(name).get();
    assert(p->pipeline != VK_NULL_HANDLE);
    return p->pipeline;
}