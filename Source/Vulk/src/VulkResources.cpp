#include "Vulk/VulkResources.h"

#include <filesystem>
#include <iostream>
#include <nlohmann/json.hpp>
#include <string>
#include <unordered_map>

#include "Vulk/VulkDepthView.h"
#include "Vulk/VulkDescriptorSetBuilder.h"
#include "Vulk/VulkDescriptorSetLayoutBuilder.h"
#include "Vulk/VulkMesh.h"
#include "Vulk/VulkPipelineBuilder.h"
#include "Vulk/VulkPointLight.h"
#include "Vulk/VulkResourceMetadata.h"

using namespace std;
namespace fs = filesystem;

VulkResources::VulkResources(Vulk &vk) : vk(vk), metadata(*getMetadata()) {
    textureSampler = VulkSampler::createImageSampler(vk);
    shadowMapSampler = VulkSampler::createShadowSampler(vk);
}

std::shared_ptr<VulkShaderModule> VulkResources::createShaderModule(ShaderType type, string const &name) {
    fs::path subdir;
    char const *suffix;
    std::unordered_map<std::string, std::shared_ptr<VulkShaderModule>> *shaders_map;
    switch (type) {
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
        VULK_THROW("Invalid shader type");
    };

    fs::path path = getResourcesDir() / "Source" / "Shaders" / subdir / (name + suffix);
    auto shaderCode = readFileIntoMem(path.string());
    VkShaderModule shaderModule = vk.createShaderModule(shaderCode);
    auto sm = make_shared<VulkShaderModule>(vk, shaderModule);
    shaders_map->insert({name, sm});
    return sm;
}

shared_ptr<VulkModel> VulkResources::getModel(ModelDef &modelDef) {
    string name = modelDef.name;
    if (!models.contains(name)) {
        shared_ptr<VulkMaterialTextures> textures = getMaterialTextures(modelDef.material->name);
        auto mr = make_shared<VulkModel>(vk, getMesh(*modelDef.mesh), textures, getMaterial(modelDef.material->name));
        models[name] = mr;
    }
    return models[name];
}

std::shared_ptr<VulkPipeline> VulkResources::loadPipeline(VkRenderPass renderPass, VkExtent2D extent, std::string const &name) {
    if (pipelines.contains(name)) {
        return pipelines[name];
    }
    PipelineDef &def = *metadata.pipelines.at(name);
    shared_ptr<VulkDescriptorSetLayout> descriptorSetLayout;
    uint32_t dsHash = def.descriptorSet.hash();
    if (descriptorSetLayoutCache.contains(dsHash)) {
        descriptorSetLayout = descriptorSetLayoutCache[dsHash];
    } else {
        VulkDescriptorSetLayoutBuilder dslb(vk);
        for (auto &[stage, bindings] : def.descriptorSet.uniformBuffers) {
            for (auto &binding : bindings) {
                dslb.addUniformBuffer(stage, binding);
            }
        }
        for (auto &[stage, bindings] : def.descriptorSet.storageBuffers) {
            for (auto &binding : bindings) {
                dslb.addStorageBuffer(stage, binding);
            }
        }
        for (auto &[stage, bindings] : def.descriptorSet.imageSamplers) {
            for (auto &binding : bindings) {
                dslb.addImageSampler(stage, binding);
            }
        }
        descriptorSetLayout = dslb.build();
        descriptorSetLayoutCache[dsHash] = descriptorSetLayout;
    }

    VulkPipelineBuilder pb(vk);
    pb.addvertShaderStage(getvertShader(def.vertShader->name))
        .addFragmentShaderStage(getFragmentShader(def.fragShader->name))
        .addVulkVertexInput(def.vertexInputBinding)
        .setDepthTestEnabled(def.depthTestEnabled)
        .setDepthWriteEnabled(def.depthWriteEnabled)
        .setDepthCompareOp(def.depthCompareOp)
        .setPrimitiveTopology(def.primitiveTopology)
        .setLineWidth(1.0f)
        .setScissor(extent)
        .setViewport(extent)
        // .setCullMode(VK_CULL_MODE_BACK_BIT)
        .setDepthCompareOp(VK_COMPARE_OP_LESS)
        .setStencilTestEnabled(false)
        .setBlending(def.blending.enabled, def.blending.getColorMask());
    if (def.geomShader) {
        pb.addGeometryShaderStage(getGeometryShader(def.geomShader->name));
    }

    auto p = pb.build(renderPass, descriptorSetLayout);
    pipelines[name] = p;
    return p;
}

std::shared_ptr<VulkActor> VulkResources::createActorFromPipeline(ActorDef const &actorDef, shared_ptr<PipelineDef> pipelineDef, shared_ptr<VulkScene> scene) {
    auto const &dsDef = pipelineDef->descriptorSet;
    VulkDescriptorSetBuilder builder(vk);
    shared_ptr<VulkModel> model = getModel(*actorDef.model);
    shared_ptr<VulkFrameUBOs<glm::mat4>> xformUBOs;
    uint32_t const dsHash = dsDef.hash();
    if (descriptorSetLayoutCache.contains(dsHash)) {
        builder.setDescriptorSetLayout(descriptorSetLayoutCache[dsHash]);
    }
    for (auto &iter : dsDef.uniformBuffers) {
        VkShaderStageFlagBits stage = (VkShaderStageFlagBits)iter.first;
        for (VulkShaderUBOBinding binding : iter.second) {
            switch (binding) {
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
            case VulkShaderUBOBinding_DebugNormals:
                if (scene->debugNormalsUBO == nullptr)
                    scene->debugNormalsUBO = make_shared<VulkUniformBuffer<VulkDebugNormalsUBO>>(vk);
                builder.addUniformBuffer(*scene->debugNormalsUBO, stage, binding);
                break;
            case VulkShaderUBOBinding_DebugTangents:
                if (scene->debugTangentsUBO == nullptr)
                    scene->debugTangentsUBO = make_shared<VulkUniformBuffer<VulkDebugTangentsUBO>>(vk);
                builder.addUniformBuffer(*scene->debugTangentsUBO, stage, binding);
                break;
            case VulkShaderUBOBinding_LightViewProjUBO:
                if (scene->lightViewProjUBO == nullptr)
                    scene->lightViewProjUBO = make_shared<VulkUniformBuffer<VulkLightViewProjUBO>>(vk);
                builder.addUniformBuffer(*scene->lightViewProjUBO, stage, binding);
                break;
            default:
                VULK_THROW("Invalid UBO binding");
            }
            static_assert(VulkShaderUBOBinding_MAX == VulkShaderUBOBinding_LightViewProjUBO);
        }
    }
    // for (auto &[stage, ssbos] : dsDef.storageBuffers)
    // {
    //     for (VulkShaderSSBOBinding binding : ssbos)
    //     {
    //         switch (binding)
    //         {
    //         case VulkShaderSSBOBinding_MaxBindingID:
    //         default:
    //             VULK_THROW("Invalid SSBO binding");
    //         }
    //     }
    // }
    static_assert(VulkShaderSSBOBinding_MaxBindingID == 0);
    for (auto &[stage, samplers] : dsDef.imageSamplers) {
        for (VulkShaderTextureBinding binding : samplers) {
            switch (binding) {
            case VulkShaderTextureBinding_TextureSampler:
                builder.addBothFramesImageSampler(stage, binding, model->textures->diffuseView, textureSampler);
                break;
            case VulkShaderTextureBinding_NormalSampler:
                builder.addBothFramesImageSampler(stage, binding, model->textures->normalView, textureSampler);
                break;
            case VulkShaderTextureBinding_ShadowMapSampler:
                for (uint32_t i = 0; i < scene->shadowMapViews.size(); i++) {
                    builder.addFrameImageSampler(i, stage, binding, scene->shadowMapViews[i]->depthView, shadowMapSampler);
                }
                break;
            default:
                VULK_THROW("Invalid texture binding");
            }
        }
    }
    std::shared_ptr<VulkDescriptorSetInfo> info = builder.build();
    descriptorSetLayoutCache[dsHash] = info->descriptorSetLayout;
    return make_shared<VulkActor>(vk, model, xformUBOs, info, getPipeline(pipelineDef->name));
    static_assert(VulkShaderTextureBinding_MAX == VulkShaderTextureBinding_ShadowMapSampler);
}

std::shared_ptr<VulkScene> VulkResources::loadScene(VkRenderPass renderPass, std::string name,
                                                    std::array<std::shared_ptr<VulkDepthView>, MAX_FRAMES_IN_FLIGHT> shadowMapViews) {
    if (scenes.contains(name)) {
        return scenes[name];
    }
    SceneDef &sceneDef = *metadata.scenes.at(name);
    shared_ptr<VulkScene> scene = make_shared<VulkScene>(vk);

    scene->shadowMapViews = shadowMapViews;
    scene->camera = sceneDef.camera;
    *scene->sceneUBOs.pointLight.mappedUBO = *sceneDef.pointLights[0]; // just one light for now

    for (auto &actorDef : sceneDef.actors) {
        loadPipeline(renderPass, vk.swapChainExtent, actorDef->pipeline->name);
        scene->actors.push_back(createActorFromPipeline(*actorDef, actorDef->pipeline, scene));
    }
    return scenes[name] = scene;
}

shared_ptr<VulkUniformBuffer<VulkMaterialConstants>> VulkResources::getMaterial(string const &name) {
    if (!materialUBOs.contains(name)) {
        materialUBOs[name] = make_shared<VulkUniformBuffer<VulkMaterialConstants>>(vk, metadata.materials.at(name)->toVulkMaterialConstants());
    }
    return materialUBOs[name];
}

shared_ptr<VulkMesh> VulkResources::getMesh(MeshDef &meshDef) {
    string name = meshDef.name;
    if (meshes.contains(name)) {
        return meshes[name];
    }
    shared_ptr<VulkMesh> m;
    switch (meshDef.type) {
    case MeshDefType_Model:
        m = make_shared<VulkMesh>(VulkMesh::loadFromPath(meshDef.getModel()->path, name));
        break;
    case MeshDefType_Mesh: {
        m = meshDef.getMesh();
    } break;
    default:
        VULK_THROW("Invalid mesh type");
    }
    meshes[name] = m;
    return m;
}

shared_ptr<VulkMaterialTextures> VulkResources::getMaterialTextures(string const &name) {
    if (!materialTextures.contains(name)) {
        MaterialDef const &def = *metadata.materials.at(name);
        materialTextures[name] = make_shared<VulkMaterialTextures>();
        materialTextures[name]->diffuseView = !def.mapKd.empty() ? make_unique<VulkTextureView>(vk, def.mapKd, false) : nullptr;
        materialTextures[name]->normalView = !def.mapNormal.empty() ? make_unique<VulkTextureView>(vk, def.mapNormal, true) : nullptr;
    }
    return materialTextures[name];
}
