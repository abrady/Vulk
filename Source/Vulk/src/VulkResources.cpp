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
#include "Vulk/VulkUBO.h"

using namespace std;
namespace fs = filesystem;

VulkResources::VulkResources(Vulk& vk)
    : vk(vk)
    , metadata(*getMetadata()) {
    textureSampler = VulkSampler::createImageSampler(vk);
    shadowMapSampler = VulkSampler::createShadowSampler(vk);
}

std::shared_ptr<VulkShaderModule> VulkResources::createShaderModule(ShaderType type, string const& name) {
    fs::path subdir;
    char const* suffix;
    std::unordered_map<std::string, std::shared_ptr<VulkShaderModule>>* shaders_map;
    switch (type) {
    case Vert:
        subdir = "Vert";
        suffix = ".vertspv";
        shaders_map = &vertShaders;
        break;
    case Geom:
        subdir = "Geom";
        suffix = ".geomspv";
        shaders_map = &geomShaders;
        break;
    case Frag:
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

std::shared_ptr<VulkModel> VulkResources::getModel(ModelDef const& modelDef, BuiltPipelineDef const& pipelineDef) {
    string key = modelDef.name + ":" + pipelineDef.name;
    if (pipelineModels.contains(key)) {
        return pipelineModels.at(key);
    }
    shared_ptr<VulkMaterialTextures> textures = getMaterialTextures(modelDef.material->name);
    auto p = make_shared<VulkModel>(vk, getMesh(*modelDef.mesh), textures, getMaterial(modelDef.material->name), pipelineDef.vertInputs);
    pipelineModels[key] = p;
    return p;
}

std::shared_ptr<VulkPipeline> VulkResources::loadPipeline(VkRenderPass renderPass, VkExtent2D extent, std::string const& name) {
    if (pipelines.contains(name)) {
        return pipelines[name];
    }
    BuiltPipelineDef& def = *metadata.pipelines.at(name);
    shared_ptr<VulkDescriptorSetLayout> descriptorSetLayout;
    // TODO: possible bug here. I should just use the name.
    // uint32_t dsHash = def.descriptorSet.hash();
    if (descriptorSetLayoutCache.contains(def.descriptorSet)) {
        descriptorSetLayout = descriptorSetLayoutCache[def.descriptorSet];
    } else {
        VulkDescriptorSetLayoutBuilder dslb(vk);
        for (auto& [stage, bindings] : def.descriptorSetDef.uniformBuffers) {
            for (auto& binding : bindings) {
                vulk::VulkShaderUBOBinding::type& bindingRef = binding;
                dslb.addUniformBuffer(stage, bindingRef);
            }
        }
        for (auto& [stage, bindings] : def.descriptorSetDef.storageBuffers) {
            for (auto& binding : bindings) {
                dslb.addStorageBuffer(stage, binding);
            }
        }
        for (auto& [stage, bindings] : def.descriptorSetDef.imageSamplers) {
            for (auto& binding : bindings) {
                dslb.addImageSampler(stage, binding);
            }
        }
        descriptorSetLayout = dslb.build();
        descriptorSetLayoutCache[def.descriptorSet] = descriptorSetLayout;
    }

    VulkPipelineBuilder pb(vk);
    for (auto& pc : def.pushConstants) {
        pb.addPushConstantRange(pc.stageFlags, pc.size);
    }
    pb.addvertShaderStage(getvertShader(def.vertShader->name))
        .addFragmentShaderStage(getFragmentShader(def.fragShader->name))
        .setDepthTestEnabled(def.depthTestEnabled)
        .setDepthWriteEnabled(def.depthWriteEnabled)
        .setDepthCompareOp(def.depthCompareOp)
        .setPrimitiveTopology(def.primitiveTopology)
        .setPolygonMode(def.polygonMode)
        .setLineWidth(1.0f)
        .setScissor(extent)
        .setViewport(extent)
        .setCullMode(def.cullMode)
        .setDepthCompareOp(VK_COMPARE_OP_LESS)
        .setStencilTestEnabled(false)
        .setBlending(def.blending.enabled, def.blending.getColorMask());
    if (def.geomShader) {
        pb.addGeometryShaderStage(getGeometryShader(def.geomShader->name));
    }

    for (VulkShaderLocation input : def.vertInputs) {
        pb.addVertexInput(input);
    }

    auto p = pb.build(renderPass, descriptorSetLayout);
    pipelines[name] = p;
    return p;
}

std::shared_ptr<VulkActor> VulkResources::createActorFromPipeline(ActorDef const& actorDef, shared_ptr<BuiltPipelineDef> pipelineDef, shared_ptr<VulkScene> scene) {
    auto const& dsDef = pipelineDef->descriptorSetDef;
    VulkDescriptorSetBuilder builder(vk);
    shared_ptr<VulkModel> model = getModel(*actorDef.model, *pipelineDef);
    shared_ptr<VulkFrameUBOs<glm::mat4>> xformUBOs;
    if (descriptorSetLayoutCache.contains(pipelineDef->descriptorSet)) {
        builder.setDescriptorSetLayout(descriptorSetLayoutCache[pipelineDef->descriptorSet]);
    }
    for (auto& iter : dsDef.uniformBuffers) {
        VkShaderStageFlagBits stage = (VkShaderStageFlagBits)iter.first;
        for (vulk::VulkShaderUBOBinding::type binding : iter.second) {
            switch (binding) {
            case vulk::VulkShaderUBOBinding::Xforms:
                builder.addFrameUBOs(scene->sceneUBOs.xforms, stage, binding);
                break;
            case vulk::VulkShaderUBOBinding::MaterialUBO:
                builder.addUniformBuffer(*model->materialUBO, stage, binding);
                break;
            case vulk::VulkShaderUBOBinding::Lights:
                builder.addUniformBuffer(scene->sceneUBOs.pointLight, stage, binding);
                break;
            case vulk::VulkShaderUBOBinding::EyePos:
                builder.addFrameUBOs(scene->sceneUBOs.eyePos, stage, binding);
                break;
            case vulk::VulkShaderUBOBinding::ModelXform:
                if (!xformUBOs)
                    xformUBOs = make_shared<VulkFrameUBOs<glm::mat4>>(vk, actorDef.xform);
                builder.addFrameUBOs(*xformUBOs, stage, binding);
                break;
            case vulk::VulkShaderUBOBinding::DebugNormals:
                if (scene->debugNormalsUBO == nullptr)
                    scene->debugNormalsUBO = make_shared<VulkUniformBuffer<VulkDebugNormalsUBO>>(vk);
                builder.addUniformBuffer(*scene->debugNormalsUBO, stage, binding);
                break;
            case vulk::VulkShaderUBOBinding::DebugTangents:
                if (scene->debugTangentsUBO == nullptr)
                    scene->debugTangentsUBO = make_shared<VulkUniformBuffer<VulkDebugTangentsUBO>>(vk);
                builder.addUniformBuffer(*scene->debugTangentsUBO, stage, binding);
                break;
            case vulk::VulkShaderUBOBinding::LightViewProjUBO:
                if (scene->lightViewProjUBO == nullptr)
                    scene->lightViewProjUBO = make_shared<VulkUniformBuffer<VulkLightViewProjUBO>>(vk);
                builder.addUniformBuffer(*scene->lightViewProjUBO, stage, binding);
                break;
            case vulk::VulkShaderUBOBinding::PBRDebugUBO:
                if (scene->pbrDebugUBO == nullptr)
                    scene->pbrDebugUBO = make_shared<VulkUniformBuffer<VulkPBRDebugUBO>>(vk);
                builder.addUniformBuffer(*scene->pbrDebugUBO, stage, binding);
                break;
            case vulk::VulkShaderUBOBinding::GlobalConstantsUBO:
                if (scene->globalConstantsUBO == nullptr)
                    scene->globalConstantsUBO = make_shared<VulkUniformBuffer<VulkGlobalConstantsUBO>>(vk);
                builder.addUniformBuffer(*scene->globalConstantsUBO, stage, binding);
                break;
            default:
                VULK_THROW("Invalid UBO binding");
            }
            static_assert(vulk::VulkShaderUBOBinding::type::MAX == vulk::VulkShaderUBOBinding::type::GlobalConstantsUBO);
        }
    }
    // for (auto &[stage, ssbos] : dsDef.storageBuffers)
    // {
    //     for (vulk::VulkShaderSSBOBinding::type binding : ssbos)
    //     {
    //         switch (binding)
    //         {
    //         case vulk::VulkShaderSSBOBinding::type_MaxBindingID:
    //         default:
    //             VULK_THROW("Invalid SSBO binding");
    //         }
    //     }
    // }
    static_assert(vulk::VulkShaderSSBOBinding::type::MaxBindingID == 0);
    for (auto& [stage, samplers] : dsDef.imageSamplers) {
        for (vulk::VulkShaderTextureBinding::type binding : samplers) {
            switch (binding) {
            case vulk::VulkShaderTextureBinding::type::TextureSampler:
                builder.addBothFramesImageSampler(stage, binding, model->textures->diffuseView, textureSampler);
                break;
            case vulk::VulkShaderTextureBinding::type::NormalSampler:
                builder.addBothFramesImageSampler(stage, binding, model->textures->normalView, textureSampler);
                break;
            case vulk::VulkShaderTextureBinding::type::ShadowMapSampler:
                for (uint32_t i = 0; i < scene->shadowMapViews.size(); i++) {
                    builder.addFrameImageSampler(i, stage, binding, scene->shadowMapViews[i]->depthView, shadowMapSampler);
                }
                break;
            case vulk::VulkShaderTextureBinding::type::AmbientOcclusionSampler:
                builder.addBothFramesImageSampler(stage, binding, model->textures->ambientOcclusionView, textureSampler);
                break;
            case vulk::VulkShaderTextureBinding::type::DisplacementSampler:
                builder.addBothFramesImageSampler(stage, binding, model->textures->displacementView, textureSampler);
                break;
            case vulk::VulkShaderTextureBinding::type::MetallicSampler:
                builder.addBothFramesImageSampler(stage, binding, model->textures->metallicView, textureSampler);
                break;
            case vulk::VulkShaderTextureBinding::type::RoughnessSampler:
                builder.addBothFramesImageSampler(stage, binding, model->textures->roughnessView, textureSampler);
                break;
            default:
                VULK_THROW("Invalid texture binding");
            }
        }
    }
    std::shared_ptr<VulkDescriptorSetInfo> info = builder.build();
    descriptorSetLayoutCache[pipelineDef->descriptorSet] = info->descriptorSetLayout;
    return make_shared<VulkActor>(vk, model, xformUBOs, info, getPipeline(pipelineDef->name));
    static_assert(vulk::VulkShaderTextureBinding::type::MAX == vulk::VulkShaderTextureBinding::type::RoughnessSampler);
}

std::shared_ptr<VulkScene> VulkResources::loadScene(VkRenderPass renderPass, std::string name, std::array<std::shared_ptr<VulkDepthView>, MAX_FRAMES_IN_FLIGHT> shadowMapViews) {
    if (scenes.contains(name)) {
        return scenes[name];
    }
    SceneDef& sceneDef = *metadata.scenes.at(name);
    shared_ptr<VulkScene> scene = make_shared<VulkScene>(vk);

    scene->shadowMapViews = shadowMapViews;
    scene->camera = sceneDef.camera;
    *scene->sceneUBOs.pointLight.mappedUBO = *sceneDef.pointLights[0]; // just one light for now

    for (auto& actorDef : sceneDef.actors) {
        loadPipeline(renderPass, vk.swapChainExtent, actorDef->pipeline->name);
        scene->actors.push_back(createActorFromPipeline(*actorDef, actorDef->pipeline, scene));
    }
    return scenes[name] = scene;
}

shared_ptr<VulkUniformBuffer<VulkMaterialConstants>> VulkResources::getMaterial(string const& name) {
    if (!materialUBOs.contains(name)) {
        materialUBOs[name] = make_shared<VulkUniformBuffer<VulkMaterialConstants>>(vk, metadata.materials.at(name)->toVulkMaterialConstants());
    }
    return materialUBOs[name];
}

shared_ptr<VulkMesh> VulkResources::getMesh(MeshDef& meshDef) {
    string name = meshDef.name;
    if (meshes.contains(name)) {
        return meshes[name];
    }
    shared_ptr<VulkMesh> m;
    switch (meshDef.type) {
    case MeshDefType_Model:
        m = make_shared<VulkMesh>(VulkMesh::loadFromPath(meshDef.getModelMeshDef()->path, name));
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

shared_ptr<VulkMaterialTextures> VulkResources::getMaterialTextures(string const& name) {
    if (!materialTextures.contains(name)) {
        MaterialDef const& def = *metadata.materials.at(name);
        materialTextures[name] = make_shared<VulkMaterialTextures>();
        materialTextures[name]->diffuseView = !def.mapKd.empty() ? make_unique<VulkImageView>(vk, def.mapKd, false) : nullptr;
        materialTextures[name]->normalView = !def.mapNormal.empty() ? make_unique<VulkImageView>(vk, def.mapNormal, true) : nullptr;
        materialTextures[name]->ambientOcclusionView = !def.mapKa.empty() ? make_unique<VulkImageView>(vk, def.mapKa, true) : nullptr;
        materialTextures[name]->displacementView = !def.disp.empty() ? make_unique<VulkImageView>(vk, def.disp, true) : nullptr;
        materialTextures[name]->metallicView = !def.mapPm.empty() ? make_unique<VulkImageView>(vk, def.mapPm, true) : nullptr;
        materialTextures[name]->roughnessView = !def.mapPr.empty() ? make_unique<VulkImageView>(vk, def.mapPr, true) : nullptr;
        static_assert(vulk::VulkShaderTextureBinding::type::MAX == vulk::VulkShaderTextureBinding::type::RoughnessSampler);
    }

    return materialTextures[name];
}
