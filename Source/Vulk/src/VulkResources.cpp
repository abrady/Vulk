#include "Vulk/VulkResources.h"

#include <filesystem>
#include <iostream>
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

std::shared_ptr<VulkModel> VulkResources::getModel(ModelDef const& modelDef, PipelineDef const& pipelineDef) {
    string key = modelDef.name + ":" + pipelineDef.def.name().value();
    if (pipelineModels.contains(key)) {
        return pipelineModels.at(key);
    }
    shared_ptr<VulkMaterialTextures> textures = getMaterialTextures(modelDef.material->name);
    //     VulkModel(Vulk& vk, std::shared_ptr<VulkMesh> meshIn, std::shared_ptr<VulkMaterialTextures> texturesIn, std::shared_ptr<VulkUniformBuffer<VulkMaterialConstants>>
    //     materialUBO,
    //          std::vector<vulk::cpp2::VulkShaderLocation> const& inputs)

    auto p = make_shared<VulkModel>(vk, getMesh(*modelDef.mesh), textures, getMaterial(modelDef.material->name), pipelineDef.def.get_vertInputs());
    pipelineModels[key] = p;
    return p;
}

// these map to the same values
VkPrimitiveTopology toVkPrimitiveTopology(vulk::cpp2::VulkPrimitiveTopology primitiveTopology) {
    return static_cast<VkPrimitiveTopology>(primitiveTopology);
}

VkCompareOp toVkCompareOp(vulk::cpp2::VulkCompareOp compareOp) {
    return static_cast<VkCompareOp>(compareOp);
}

VkPolygonMode toVkPolygonMode(vulk::cpp2::VulkPolygonMode polygonMode) {
    return static_cast<VkPolygonMode>(polygonMode);
}

std::shared_ptr<VulkPipeline> VulkResources::loadPipeline(VkRenderPass renderPass, VkExtent2D extent, std::string const& name) {
    if (pipelines.contains(name)) {
        return pipelines[name];
    }
    std::shared_ptr<PipelineDef> def = metadata.pipelines.at(name);

    // build the descriptor set layout
    VulkDescriptorSetLayoutBuilder dslb(vk);
    for (auto& [stage, bindings] : def->def.get_descriptorSetDef().get_uniformBuffers()) {
        for (auto& binding : bindings) {
            vulk::cpp2::VulkShaderUBOBinding const& bindingRef = binding;
            dslb.addUniformBuffer(stage, bindingRef);
        }
    }
    for (auto& [stage, bindings] : def->def.get_descriptorSetDef().get_storageBuffers()) {
        for (auto& binding : bindings) {
            dslb.addStorageBuffer(stage, binding);
        }
    }
    for (auto& [stage, bindings] : def->def.get_descriptorSetDef().get_imageSamplers()) {
        for (auto& binding : bindings) {
            dslb.addImageSampler(stage, binding);
        }
    }
    shared_ptr<VulkDescriptorSetLayout> descriptorSetLayout = dslb.build();

    // make the pipeline itself
    VulkPipelineBuilder pb(vk, def);
    for (auto& pc : def->def.get_pushConstants()) {
        pb.addPushConstantRange(pc.get_stageFlags(), pc.get_size());
    }
    pb.addvertShaderStage(getvertShader(def->vertShader->get_name()))
        .setLineWidth(1.0f)
        .setScissor(extent)
        .setViewport(extent)
        .setDepthCompareOp(VK_COMPARE_OP_LESS)
        .setStencilTestEnabled(false);

    if (def->fragShader)
        pb.addFragmentShaderStage(getFragmentShader(def->fragShader->get_name()));
    vulk::cpp2::PipelineDef const& pd = def->def;
    if (pd.depthTestEnabled().is_set())
        pb.setDepthTestEnabled(pd.get_depthTestEnabled());
    if (pd.depthWriteEnabled().is_set())
        pb.setDepthWriteEnabled(pd.get_depthWriteEnabled());
    if (pd.depthCompareOp().is_set())
        pb.setDepthCompareOp(toVkCompareOp(pd.get_depthCompareOp()));
    if (pd.primitiveTopology().is_set())
        pb.setPrimitiveTopology(toVkPrimitiveTopology(pd.get_primitiveTopology()));
    if (pd.polygonMode().is_set())
        pb.setPolygonMode(toVkPolygonMode(pd.get_polygonMode()));
    if (pd.cullMode().is_set())
        pb.setCullMode((VkCullModeFlags)pd.get_cullMode());
    if (pd.blending().is_set())
        pb.setBlending(pd.get_blending().get_enabled(), getColorMask(pd.get_blending().get_colorWriteMask()));
    if (def->geomShader)
        pb.addGeometryShaderStage(getGeometryShader(def->geomShader->get_name()));

    for (auto input : def->def.get_vertInputs()) {
        pb.addVertexInput(input);
    }

    auto p = pb.build(renderPass, descriptorSetLayout);
    pipelines[name] = p;
    return p;
}

std::shared_ptr<VulkActor> VulkResources::createActorFromPipeline(ActorDef const& actorDef, shared_ptr<VulkPipeline> pipeline, shared_ptr<VulkScene> scene) {
    vulk::cpp2::DescriptorSetDef const& dsDef = pipeline->def->def.get_descriptorSetDef();
    VulkDescriptorSetBuilder builder(vk);
    PipelineDef const& pd = *pipeline->def;
    shared_ptr<VulkModel> model = getModel(*actorDef.model, pd);
    shared_ptr<VulkFrameUBOs<glm::mat4>> xformUBOs;
    builder.setDescriptorSetLayout(pipeline->descriptorSetLayout);
    for (auto& iter : dsDef.get_uniformBuffers()) {
        VkShaderStageFlagBits stage = (VkShaderStageFlagBits)iter.first;
        for (vulk::cpp2::VulkShaderUBOBinding binding : iter.second) {
            switch (binding) {
            case vulk::cpp2::VulkShaderUBOBinding::Xforms:
                builder.addFrameUBOs(scene->sceneUBOs.xforms, stage, binding);
                break;
            case vulk::cpp2::VulkShaderUBOBinding::MaterialUBO:
                builder.addUniformBuffer(*model->materialUBO, stage, binding);
                break;
            case vulk::cpp2::VulkShaderUBOBinding::Lights:
                builder.addUniformBuffer(scene->sceneUBOs.pointLight, stage, binding);
                break;
            case vulk::cpp2::VulkShaderUBOBinding::EyePos:
                builder.addFrameUBOs(scene->sceneUBOs.eyePos, stage, binding);
                break;
            case vulk::cpp2::VulkShaderUBOBinding::ModelXform:
                if (!xformUBOs)
                    xformUBOs = make_shared<VulkFrameUBOs<glm::mat4>>(vk, actorDef.xform);
                builder.addFrameUBOs(*xformUBOs, stage, binding);
                break;
            case vulk::cpp2::VulkShaderUBOBinding::DebugNormals:
                if (scene->debugNormalsUBO == nullptr)
                    scene->debugNormalsUBO = make_shared<VulkUniformBuffer<VulkDebugNormalsUBO>>(vk);
                builder.addUniformBuffer(*scene->debugNormalsUBO, stage, binding);
                break;
            case vulk::cpp2::VulkShaderUBOBinding::DebugTangents:
                if (scene->debugTangentsUBO == nullptr)
                    scene->debugTangentsUBO = make_shared<VulkUniformBuffer<VulkDebugTangentsUBO>>(vk);
                builder.addUniformBuffer(*scene->debugTangentsUBO, stage, binding);
                break;
            case vulk::cpp2::VulkShaderUBOBinding::LightViewProjUBO:
                if (scene->lightViewProjUBO == nullptr)
                    scene->lightViewProjUBO = make_shared<VulkUniformBuffer<VulkLightViewProjUBO>>(vk);
                builder.addUniformBuffer(*scene->lightViewProjUBO, stage, binding);
                break;
            case vulk::cpp2::VulkShaderUBOBinding::PBRDebugUBO:
                if (scene->pbrDebugUBO == nullptr)
                    scene->pbrDebugUBO = make_shared<VulkUniformBuffer<VulkPBRDebugUBO>>(vk);
                builder.addUniformBuffer(*scene->pbrDebugUBO, stage, binding);
                break;
            case vulk::cpp2::VulkShaderUBOBinding::GlobalConstantsUBO:
                if (scene->globalConstantsUBO == nullptr)
                    scene->globalConstantsUBO = make_shared<VulkUniformBuffer<VulkGlobalConstantsUBO>>(vk);
                builder.addUniformBuffer(*scene->globalConstantsUBO, stage, binding);
                break;
            default:
                VULK_THROW("Invalid UBO binding");
            }
            static_assert((int)vulk::cpp2::VulkShaderUBOBinding::MAX == 21);
        }
    }
    // for (auto &[stage, ssbos] : dsDef.storageBuffers)
    // {
    //     for (vulk::cpp2::VulkShaderSSBOBinding binding : ssbos)
    //     {
    //         switch (binding)
    //         {
    //         case vulk::cpp2::VulkShaderSSBOBinding::type_MaxBindingID:
    //         default:
    //             VULK_THROW("Invalid SSBO binding");
    //         }
    //     }
    // }
    static_assert((int)vulk::cpp2::VulkShaderSSBOBinding::MaxBindingID == 0);
    for (auto& [stage, samplers] : dsDef.get_imageSamplers()) {
        for (vulk::cpp2::VulkShaderTextureBinding binding : samplers) {
            switch (binding) {
            case vulk::cpp2::VulkShaderTextureBinding::TextureSampler:
                builder.addBothFramesImageSampler(stage, binding, model->textures->diffuseView, textureSampler);
                break;
            case vulk::cpp2::VulkShaderTextureBinding::NormalSampler:
                builder.addBothFramesImageSampler(stage, binding, model->textures->normalView, textureSampler);
                break;
            case vulk::cpp2::VulkShaderTextureBinding::ShadowMapSampler:
                for (uint32_t i = 0; i < scene->shadowMapViews.size(); i++) {
                    builder.addFrameImageSampler(i, stage, binding, scene->shadowMapViews[i]->depthView, shadowMapSampler);
                }
                break;
            case vulk::cpp2::VulkShaderTextureBinding::AmbientOcclusionSampler:
                builder.addBothFramesImageSampler(stage, binding, model->textures->ambientOcclusionView, textureSampler);
                break;
            case vulk::cpp2::VulkShaderTextureBinding::DisplacementSampler:
                builder.addBothFramesImageSampler(stage, binding, model->textures->displacementView, textureSampler);
                break;
            case vulk::cpp2::VulkShaderTextureBinding::MetallicSampler:
                builder.addBothFramesImageSampler(stage, binding, model->textures->metallicView, textureSampler);
                break;
            case vulk::cpp2::VulkShaderTextureBinding::RoughnessSampler:
                builder.addBothFramesImageSampler(stage, binding, model->textures->roughnessView, textureSampler);
                break;
            default:
                VULK_THROW("Invalid texture binding");
            }
        }
    }
    std::shared_ptr<VulkDescriptorSetInfo> info = builder.build();
    return make_shared<VulkActor>(vk, model, xformUBOs, info, pipeline);
    static_assert((int)vulk::cpp2::VulkShaderTextureBinding::MAX == 19);
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
        auto pipeline = loadPipeline(renderPass, vk.swapChainExtent, actorDef->pipeline->def.get_name());
        scene->actors.push_back(createActorFromPipeline(*actorDef, pipeline, scene));
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
    case vulk::cpp2::MeshDefType::Model:
        m = make_shared<VulkMesh>(VulkMesh::loadFromPath(meshDef.getModelMeshDef()->path, name));
        break;
    case vulk::cpp2::MeshDefType::Mesh: {
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
        static_assert((int)vulk::cpp2::VulkShaderTextureBinding::MAX == 19);
    }

    return materialTextures[name];
}
