#include "Vulk/VulkResources.h"

#include <filesystem>
#include <iostream>
#include <string>
#include <unordered_map>

#include "Vulk/VulkDeferredRenderpass.h"
#include "Vulk/VulkDepthView.h"
#include "Vulk/VulkDescriptorSetBuilder.h"
#include "Vulk/VulkDescriptorSetLayoutBuilder.h"
#include "Vulk/VulkMesh.h"
#include "Vulk/VulkPipelineBuilder.h"
#include "Vulk/VulkResourceMetadata.h"
#include "Vulk/VulkUBO.h"

using namespace std;
namespace fs = filesystem;

DECLARE_FILE_LOGGER();

VulkResources::VulkResources(Vulk& vk) : vk(vk), metadata(getMetadata()) {
    textureSampler   = VulkSampler::createImageSampler(vk);
    shadowMapSampler = VulkSampler::createShadowSampler(vk);
}

VulkResources::VulkResources(Vulk& vk, std::shared_ptr<Metadata> metadata) : vk(vk), metadata(metadata) {
    textureSampler   = VulkSampler::createImageSampler(vk);
    shadowMapSampler = VulkSampler::createShadowSampler(vk);
}

std::shared_ptr<VulkResources> VulkResources::loadFromProject(Vulk& vk, std::filesystem::path projectFile) {
    std::shared_ptr<Metadata> metadata = std::make_shared<Metadata>();
    findAndProcessMetadata(projectFile.parent_path() / "Assets", *metadata);
    return std::make_shared<VulkResources>(vk, metadata);
}

std::shared_ptr<const VulkShaderModule> VulkResources::createShaderModule(ShaderType type, string const& name) const {
    fs::path subdir;
    char const* suffix;
    std::unordered_map<std::string, std::shared_ptr<const VulkShaderModule>>* shaders_map;
    switch (type) {
        case Vert:
            subdir      = "Vert";
            suffix      = ".vertspv";
            shaders_map = &vertShaders;
            break;
        case Geom:
            subdir      = "Geom";
            suffix      = ".geomspv";
            shaders_map = &geomShaders;
            break;
        case Frag:
            subdir      = "Frag";
            suffix      = ".fragspv";
            shaders_map = &fragShaders;
            break;
        default:
            VULK_THROW("Invalid shader type");
    };

    fs::path path               = metadata->assetsDir / "Shaders" / subdir / (name + suffix);
    auto shaderCode             = readFileIntoMem(path.string());
    VkShaderModule shaderModule = vk.createShaderModule(shaderCode);
    auto sm                     = make_shared<VulkShaderModule>(vk, shaderModule);
    shaders_map->insert({name, sm});
    return sm;
}

std::shared_ptr<const VulkModel> VulkResources::getModel(ModelDef const& modelDef, PipelineDef const& pipelineDef) {
    string key = modelDef.name + ":" + pipelineDef.def.name().value();
    if (pipelineModels.contains(key)) {
        return pipelineModels.at(key);
    }
    shared_ptr<const VulkMaterialTextures> textures = getMaterialTextures(modelDef.material->name);
    auto p                                          = make_shared<VulkModel>(vk,
                                    getMesh(*modelDef.mesh),
                                    textures,
                                    getMaterial(modelDef.material->name),
                                    pipelineDef.def.get_vertInputs());
    pipelineModels[key]                             = p;
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

std::shared_ptr<const VulkDescriptorSetLayout> VulkResources::buildDescriptorSetLayoutFromPipeline(std::string name) {
    std::shared_ptr<const PipelineDef> def = metadata->pipelines.at(name);

    // build the descriptor set layout
    VulkDescriptorSetLayoutBuilder dslb(vk);
    auto dsdef = def->def.get_descriptorSetDef();
    for (auto& [stage, bindings] : dsdef.get_uniformBuffers()) {
        for (auto& binding : bindings) {
            vulk::cpp2::VulkShaderUBOBinding const& bindingRef = binding;
            dslb.addUniformBuffer(stage, bindingRef);
        }
    }
    for (auto& [stage, bindings] : dsdef.get_storageBuffers()) {
        for (auto& binding : bindings) {
            dslb.addStorageBuffer(stage, binding);
        }
    }
    for (auto& [stage, bindings] : dsdef.get_imageSamplers()) {
        for (auto& binding : bindings) {
            dslb.addImageSampler(stage, binding);
        }
    }
    for (auto& [stage, bindingDefs] : dsdef.get_inputAttachments()) {
        for (auto& bd : bindingDefs) {
            dslb.addInputAttachment(stage, bd.get_binding());
        }
    }

    return dslb.build();
}

std::shared_ptr<const VulkPipeline> VulkResources::loadPipeline(VkRenderPass renderPass,
                                                                VkExtent2D extent,
                                                                std::string const& name) {
    if (pipelines.contains(name)) {
        return pipelines[name];
    }

    // make the pipeline itself
    std::shared_ptr<const PipelineDef> def = metadata->pipelines.at(name);
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
    if (def->def.subpass().is_set())
        pb.setSubpass(def->def.get_subpass());
    if (def->def.colorBlends().is_set()) {
        auto blends = def->def.get_colorBlends();
        for (auto colorBlends : blends) {
            pb.addColorBlendAttachment(colorBlends.get_enabled(), getColorMask(colorBlends.get_colorWriteMask()));
        }
    }
    if (def->geomShader)
        pb.addGeometryShaderStage(getGeometryShader(def->geomShader->get_name()));

    for (auto input : def->def.get_vertInputs()) {
        pb.addVertexInput(input);
    }

    std::shared_ptr<const VulkDescriptorSetLayout> descriptorSetLayout = buildDescriptorSetLayoutFromPipeline(name);
    // build, save and return the pipeline
    auto p          = pb.build(renderPass, descriptorSetLayout);
    pipelines[name] = p;
    return p;
}

/**
 * @brief Create a descriptor set info from a pipeline.
 * scene, model, and deferredRenderpass are used to create the descriptor set info
 * if they apply to the pipeline (e.g. if the pipeline uses a UBO for the model, the model must be provided)
 */
shared_ptr<const VulkDescriptorSetInfo> VulkResources::createDSInfoFromPipeline(
    VulkPipeline const& pipeline,
    VulkScene const* scene,
    VulkModel const* model,
    ActorDef const* actorDef,
    vulk::VulkDeferredRenderpass const* deferredRenderpass) {
    logger->info("Creating actor from pipeline def {}", pipeline.def->def.get_name());
    vulk::cpp2::DescriptorSetDef const& dsDef = pipeline.def->def.get_descriptorSetDef();
    VulkDescriptorSetBuilder dsBuilder(vk);

    dsBuilder.setDescriptorSetLayout(pipeline.descriptorSetLayout);
    for (auto& iter : dsDef.get_uniformBuffers()) {
        VkShaderStageFlagBits stage = (VkShaderStageFlagBits)iter.first;
        for (vulk::cpp2::VulkShaderUBOBinding binding : iter.second) {
            switch (binding) {
                case vulk::cpp2::VulkShaderUBOBinding::Xforms:
                    dsBuilder.addFrameUBOs(scene->sceneUBOs.xforms, stage, binding);
                    break;
                case vulk::cpp2::VulkShaderUBOBinding::MaterialUBO:
                    dsBuilder.addUniformBuffer(*model->materialUBO, stage, binding);
                    break;
                case vulk::cpp2::VulkShaderUBOBinding::Lights:
                    dsBuilder.addUniformBuffer(scene->sceneUBOs.lightsUBO, stage, binding);
                    break;
                case vulk::cpp2::VulkShaderUBOBinding::EyePos:
                    dsBuilder.addFrameUBOs(scene->sceneUBOs.eyePos, stage, binding);
                    break;
                case vulk::cpp2::VulkShaderUBOBinding::ModelXform:
                    if (!model->xformUBOs)
                        model->xformUBOs = make_shared<VulkFrameUBOs<glm::mat4>>(vk, actorDef->xform);
                    dsBuilder.addFrameUBOs(*model->xformUBOs, stage, binding);
                    break;
                case vulk::cpp2::VulkShaderUBOBinding::DebugNormals:
                    if (scene->debugNormalsUBO == nullptr) {
                        scene->debugNormalsUBO = make_shared<VulkUniformBuffer<VulkDebugNormalsUBO>>(vk);
                    }
                    dsBuilder.addUniformBuffer(*scene->debugNormalsUBO, stage, binding);
                    break;
                case vulk::cpp2::VulkShaderUBOBinding::DebugTangents:
                    if (scene->debugTangentsUBO == nullptr)
                        scene->debugTangentsUBO = make_shared<VulkUniformBuffer<VulkDebugTangentsUBO>>(vk);
                    dsBuilder.addUniformBuffer(*scene->debugTangentsUBO, stage, binding);
                    break;
                case vulk::cpp2::VulkShaderUBOBinding::LightViewProjUBO:
                    if (scene->lightViewProjUBO == nullptr)
                        scene->lightViewProjUBO = make_shared<VulkUniformBuffer<VulkLightViewProjUBO>>(vk);
                    dsBuilder.addUniformBuffer(*scene->lightViewProjUBO, stage, binding);
                    break;
                case vulk::cpp2::VulkShaderUBOBinding::PBRDebugUBO:
                    if (scene->pbrDebugUBO == nullptr)
                        scene->pbrDebugUBO = make_shared<VulkUniformBuffer<VulkPBRDebugUBO>>(vk);
                    dsBuilder.addUniformBuffer(*scene->pbrDebugUBO, stage, binding);
                    break;
                case vulk::cpp2::VulkShaderUBOBinding::GlobalConstantsUBO:
                    if (scene->globalConstantsUBO == nullptr)
                        scene->globalConstantsUBO = make_shared<VulkUniformBuffer<VulkGlobalConstantsUBO>>(vk);
                    dsBuilder.addUniformBuffer(*scene->globalConstantsUBO, stage, binding);
                    break;
                case vulk::cpp2::VulkShaderUBOBinding::InvViewProjUBO:
                    if (scene->invViewProjUBO == nullptr)
                        scene->invViewProjUBO = make_shared<VulkUniformBuffer<glm::mat4>>(vk);
                    dsBuilder.addUniformBuffer(*scene->invViewProjUBO, stage, binding);
                    break;
                default:
                    VULK_THROW("Invalid UBO binding");
            }
            static_assert((int)TEnumTraits<::vulk::cpp2::VulkShaderUBOBinding>::max() == 27);
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
                    dsBuilder.addBothFramesImageSampler(stage, binding, model->textures->diffuseView, textureSampler);
                    break;
                case vulk::cpp2::VulkShaderTextureBinding::NormalSampler:
                    dsBuilder.addBothFramesImageSampler(stage, binding, model->textures->normalView, textureSampler);
                    break;
                case vulk::cpp2::VulkShaderTextureBinding::ShadowMapSampler:
                    for (uint32_t i = 0; i < scene->shadowMapViews.size(); i++) {
                        dsBuilder.addFrameImageSampler(i, stage, binding, scene->shadowMapViews[i]->depthView, shadowMapSampler);
                    }
                    break;
                case vulk::cpp2::VulkShaderTextureBinding::AmbientOcclusionSampler:
                    dsBuilder.addBothFramesImageSampler(stage, binding, model->textures->ambientOcclusionView, textureSampler);
                    break;
                case vulk::cpp2::VulkShaderTextureBinding::DisplacementSampler:
                    dsBuilder.addBothFramesImageSampler(stage, binding, model->textures->displacementView, textureSampler);
                    break;
                case vulk::cpp2::VulkShaderTextureBinding::MetallicSampler:
                    dsBuilder.addBothFramesImageSampler(stage, binding, model->textures->metallicView, textureSampler);
                    break;
                case vulk::cpp2::VulkShaderTextureBinding::RoughnessSampler:
                    dsBuilder.addBothFramesImageSampler(stage, binding, model->textures->roughnessView, textureSampler);
                    break;
                case vulk::cpp2::VulkShaderTextureBinding::CubemapSampler:
                    dsBuilder.addBothFramesImageSampler(stage, binding, model->textures->cubemapView, textureSampler);
                    break;
                default:
                    VULK_THROW("Invalid texture binding");
            }
        }
    }
    static_assert(TEnumTraits<::vulk::cpp2::VulkShaderTextureBinding>::max() ==
                  vulk::cpp2::VulkShaderTextureBinding::CubemapSampler);

    for (auto& [stage, inputAttachments] : dsDef.get_inputAttachments()) {
        for (vulk::cpp2::DescriptorSetInputAttachmentDef inputDef : inputAttachments) {
            vulk::cpp2::GBufBinding binding      = inputDef.get_binding();
            vulk::cpp2::GBufInputAtmtIdx atmtIdx = inputDef.get_atmtIdx();
            VULK_ASSERT(deferredRenderpass->geoBufs);
            vulk::VulkDeferredRenderpass::VulkGBufs& gbufs           = *deferredRenderpass->geoBufs;
            vulk::VulkDeferredRenderpass::DeferredImage const* image = gbufs.imageFromInput(atmtIdx);
            dsBuilder.addInputAttachment(stage, binding, image->view);
        }
        static_assert(TEnumTraits<::vulk::cpp2::GBufAtmtIdx>::max() == vulk::cpp2::GBufAtmtIdx::Depth);
    }

    return dsBuilder.build();
}

// TODO: hypothetically the same descriptor set could get used for multiple actors
// we'll get to that eventually, maybe, it's not really a big deal as DSs are cheap
// or so I've been told.
shared_ptr<const VulkActor> VulkResources::createActorFromPipeline(ActorDef const& actorDef,
                                                                   shared_ptr<const VulkPipeline> pipeline,
                                                                   VulkScene const* scene,
                                                                   vulk::VulkDeferredRenderpass const* deferredRenderpass) {
    logger->info("Creating actor from def {}, pipeline def {}", actorDef.def.get_name(), pipeline->def->def.get_name());
    shared_ptr<const VulkModel> model = getModel(*actorDef.model, *pipeline->def);
    shared_ptr<const VulkDescriptorSetInfo> info =
        createDSInfoFromPipeline(*pipeline, scene, model.get(), &actorDef, deferredRenderpass);
    return make_shared<VulkActor>(vk, model, info, pipeline);
}

std::shared_ptr<VulkScene> VulkResources::loadScene(
    std::string name,
    std::array<std::shared_ptr<VulkDepthView>, MAX_FRAMES_IN_FLIGHT> shadowMapViews) const {
    if (scenes.contains(name)) {
        logger->info("Returning cached scene {}", name);
        return scenes[name];
    }
    logger->info("Loading scene {}", name);
    SceneDef& sceneDef          = *metadata->scenes.at(name);
    shared_ptr<VulkScene> scene = make_shared<VulkScene>(vk, metadata->scenes.at(name));

    scene->shadowMapViews = shadowMapViews;
    scene->camera         = sceneDef.camera;
    VULK_ASSERT(sceneDef.pointLights.size() <= (int)vulk::cpp2::VulkLights::NumLights);
    for (size_t i = 0; i < sceneDef.pointLights.size(); i++) {
        scene->sceneUBOs.lightsUBO.mappedUBO->lights[i] = *sceneDef.pointLights[i];
    }

    // for (auto& actorDef : sceneDef.actors) {
    //     auto pipeline = loadPipeline(renderPass, vk.swapChainExtent, actorDef->pipeline.def.get_name());
    //     scene->actors.push_back(createActorFromPipeline(*actorDef, pipeline, scene));
    // }
    return scenes[name] = scene;
}

shared_ptr<const VulkUniformBuffer<VulkMaterialConstants>> VulkResources::getMaterial(string const& name) {
    if (!materialUBOs.contains(name)) {
        materialUBOs[name] =
            make_shared<VulkUniformBuffer<VulkMaterialConstants>>(vk, metadata->materials.at(name)->toVulkMaterialConstants());
    }
    return materialUBOs[name];
}

shared_ptr<const VulkMesh> VulkResources::getMesh(MeshDef& meshDef) {
    string name = meshDef.name;
    if (meshes.contains(name)) {
        return meshes[name];
    }
    shared_ptr<const VulkMesh> m;
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

shared_ptr<const VulkMaterialTextures> VulkResources::getMaterialTextures(string const& name) {
    if (!materialTextures.contains(name)) {
        MaterialDef const& def  = *metadata->materials.at(name);
        auto p                  = make_shared<VulkMaterialTextures>();
        p->diffuseView          = !def.mapKd.empty() ? make_unique<VulkImageView>(vk, def.mapKd, false) : nullptr;
        p->normalView           = !def.mapNormal.empty() ? make_unique<VulkImageView>(vk, def.mapNormal, true) : nullptr;
        p->ambientOcclusionView = !def.mapKa.empty() ? make_unique<VulkImageView>(vk, def.mapKa, true) : nullptr;
        p->displacementView     = !def.disp.empty() ? make_unique<VulkImageView>(vk, def.disp, true) : nullptr;
        p->metallicView         = !def.mapPm.empty() ? make_unique<VulkImageView>(vk, def.mapPm, true) : nullptr;
        p->roughnessView        = !def.mapPr.empty() ? make_unique<VulkImageView>(vk, def.mapPr, true) : nullptr;
        p->cubemapView          = !def.cubemapImgs[0].empty() ? VulkImageView::createCubemapView(vk, def.cubemapImgs) : nullptr;

        materialTextures[name] = p;
        static_assert(TEnumTraits<::vulk::cpp2::VulkShaderTextureBinding>::max() ==
                      vulk::cpp2::VulkShaderTextureBinding::CubemapSampler);
    }

    return materialTextures[name];
}
