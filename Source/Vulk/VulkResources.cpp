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
#include "VulkResourceMetadata.h"

using namespace std;
namespace fs = filesystem;

VulkResources::VulkResources(Vulk &vk)
    : vk(vk), metadata(*getMetadata())
{
    textureSampler = std::make_shared<VulkSampler>(vk);
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

    for (auto &[stage, bindings] : def.descriptorSet.uniformBuffers)
    {
        for (auto &binding : bindings)
        {
            dslb.addUniformBuffer(stage, binding);
        }
    }
    for (auto &[stage, bindings] : def.descriptorSet.storageBuffers)
    {
        for (auto &binding : bindings)
        {
            dslb.addStorageBuffer(stage, binding);
        }
    }
    for (auto &[stage, bindings] : def.descriptorSet.imageSamplers)
    {
        for (auto &binding : bindings)
        {
            dslb.addImageSampler(stage, binding);
        }
    }
    shared_ptr<VulkDescriptorSetLayout> descriptorSetLayout = dslb.build();

    auto pb = VulkPipelineBuilder(vk)
                  .addVertexShaderStage(getVertexShader(def.vertexShader->name))
                  .addFragmentShaderStage(getFragmentShader(def.fragmentShader->name))
                  .addVulkVertexInput(def.vertexInputBinding)
                  .setPrimitiveTopology(def.primitiveTopology)
                  .setLineWidth(1.0f)
                  .setCullMode(VK_CULL_MODE_BACK_BIT)
                  .setDepthTestEnabled(true)
                  .setDepthWriteEnabled(true)
                  .setDepthCompareOp(VK_COMPARE_OP_LESS)
                  .setStencilTestEnabled(false)
                  .setBlendingEnabled(true);
    if (def.geometryShader)
        pb.addGeometryShaderStage(getGeometryShader(def.geometryShader->name));

    auto p = pb.build(descriptorSetLayout);
    pipelines[name] = p;
    return p;
}

std::shared_ptr<VulkActor> VulkResources::createActorFromPipeline(ActorDef const &actorDef, shared_ptr<PipelineDef> pipelineDef, shared_ptr<VulkScene> scene)
{
    auto const &dsDef = pipelineDef->descriptorSet;
    VulkDescriptorSetBuilder builder(vk);
    shared_ptr<VulkModel> model = getModel(*actorDef.model);
    shared_ptr<VulkFrameUBOs<glm::mat4>> xformUBOs;
    for (auto &iter : dsDef.uniformBuffers)
    {
        VkShaderStageFlagBits stage = (VkShaderStageFlagBits)iter.first;
        for (VulkShaderUBOBindings binding : iter.second)
        {
            switch (binding)
            {
            case VulkShaderUBOBinding_Xforms:
                builder.addFrameUBOs(scene->sceneUBOs.xforms, stage, binding);
                break;
            case VulkShaderUBOBinding_MaterialUBO:
                builder.addUniformBuffer(*model->materialUBO, stage, binding);
                break;
            case VulkShaderUBOBinding_Lights:
                builder.addUniformBuffer(scene->sceneUBOs.pointLight, stage, binding);
                break;
            case VulkShaderUBOBinding_EyePos:
                builder.addFrameUBOs(scene->sceneUBOs.eyePos, stage, binding);
                break;
            case VulkShaderUBOBinding_ModelXform:
                if (!xformUBOs)
                    xformUBOs = make_shared<VulkFrameUBOs<glm::mat4>>(vk, actorDef.xform);
                builder.addFrameUBOs(*xformUBOs, stage, binding);
                break;
            default:
                throw runtime_error("Invalid UBO binding");
            }
        }
    }
    // for (auto &[stage, ssbos] : dsDef.storageBuffers)
    // {
    //     for (VulkShaderSSBOBindings binding : ssbos)
    //     {
    //         switch (binding)
    //         {
    //         case VulkShaderSSBOBinding_MaxBindingID:
    //         default:
    //             throw runtime_error("Invalid SSBO binding");
    //         }
    //     }
    // }
    static_assert(VulkShaderSSBOBinding_MaxBindingID == 0);
    for (auto &[stage, samplers] : dsDef.imageSamplers)
    {
        for (VulkShaderTextureBindings binding : samplers)
        {
            switch (binding)
            {
            case VulkShaderTextureBinding_TextureSampler:
                builder.addImageSampler(stage, binding, model->textures->diffuseView, textureSampler);
                break;
            case VulkShaderTextureBinding_NormalSampler:
                builder.addImageSampler(stage, binding, model->textures->normalView, textureSampler);
                break;
            default:
                throw runtime_error("Invalid texture binding");
            }
        }
    }
    return make_shared<VulkActor>(vk, model, xformUBOs, builder.build(), getPipeline(pipelineDef->name));
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
        scene->actors.push_back(createActorFromPipeline(*actorDef, actorDef->pipeline, scene));
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

shared_ptr<VulkPipeline> VulkResources::getPipeline(string const &name)
{
    if (pipelines.contains(name))
    {
        return pipelines[name];
    }
    auto pipelineDef = metadata.pipelines.at(name);
    return loadPipeline(*pipelineDef);
}