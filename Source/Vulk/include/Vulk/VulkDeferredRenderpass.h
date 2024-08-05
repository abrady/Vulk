#pragma once

#include <vulkan/vulkan.h>

#include "Vulk/ClassNonCopyableNonMovable.h"
#include "Vulk/Vulk.h"
#include "Vulk/VulkDescriptorSet.h"
#include "Vulk/VulkDescriptorSetBuilder.h"
#include "Vulk/VulkDescriptorSetLayout.h"
#include "Vulk/VulkImageView.h"
#include "Vulk/VulkPipeline.h"
#include "Vulk/VulkResources.h"
#include "Vulk/VulkSampler.h"

namespace vulk {

constexpr uint32_t NumGeoBufs = TEnumTraits<vulk::cpp2::VulkGBufAttachment>::size;
class VulkDeferredRenderpass : public ClassNonCopyableNonMovable {
    inline static std::shared_ptr<spdlog::logger> logger = VulkLogger::CreateLogger("VulkDeferredRenderpass");

    using VulkGBufAttachment = vulk::cpp2::VulkGBufAttachment;
    using VulkShaderTextureBinding = vulk::cpp2::VulkShaderTextureBinding;
    using VulkShaderUBOBinding = vulk::cpp2::VulkShaderUBOBinding;

    class VulkDeferredImage : public ClassNonCopyableNonMovable {
       public:
        Vulk& vk;
        std::shared_ptr<VulkImageView> view;
        VkFormat format;

        VulkDeferredImage(Vulk& vkIn, VkFormat formatIn, bool isDepth) : vk(vkIn), format(formatIn) {
            VkImage image;
            VkDeviceMemory imageMemory;
            VkImageView imageView;

            // VK_IMAGE_USAGE_SAMPLED_BIT so that we can sample from the image in the shader
            if (isDepth) {
                vk.createImage(vk.swapChainExtent.width, vk.swapChainExtent.height, format, VK_IMAGE_TILING_OPTIMAL,
                               VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
                               VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, image, imageMemory);
                imageView = vk.createImageView(image, format, VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT);
            } else {
                vk.createImage(vk.swapChainExtent.width, vk.swapChainExtent.height, format, VK_IMAGE_TILING_OPTIMAL,
                               VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
                               VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, image, imageMemory);
                imageView = vk.createImageView(image, format, VK_IMAGE_ASPECT_COLOR_BIT);
            }
            view = std::make_shared<VulkImageView>(vk, image, imageMemory, imageView);
        }
    };

    class VulkGBufs : public ClassNonCopyableNonMovable {
       public:
        Vulk& vk;
        std::unordered_map<VulkGBufAttachment, std::shared_ptr<VulkDeferredImage>> gbufs;
        std::array<VkImageView, NumGeoBufs> gbufViews;

        // normal could also be: VK_FORMAT_R16G16B16A16_SFLOAT
        // depth could be: VK_FORMAT_D24_UNORM_S8_UINT - could pack data in the stencil buffer
        // material could be more compact: VK_FORMAT_R4G4B4A4_UNORM or VK_FORMAT_R5G5B5A1_UNORM

        VulkGBufs(Vulk& vkIn) : vk(vkIn) {
            std::pair<VulkGBufAttachment, VkFormat> bindings[] = {
                {VulkGBufAttachment::Normal, VK_FORMAT_R16G16_SFLOAT},  // hemioct-encoded normal
                {VulkGBufAttachment::Depth, VK_FORMAT_D32_SFLOAT},      // vk.findDepthFormat()},
                {VulkGBufAttachment::Albedo, VK_FORMAT_R8G8B8A8_UNORM},
                {VulkGBufAttachment::Material, VK_FORMAT_R8G8B8A8_UNORM},
            };
            for (auto& [attachment, format] : bindings) {
                bool isDepth = attachment == VulkGBufAttachment::Depth;
                auto vdi = std::make_shared<VulkDeferredImage>(vk, format, isDepth);
                gbufs[attachment] = vdi;
                gbufViews[(int)attachment] = vdi->view->imageView;
                logger->debug("Created GBuf attachment: {} : image {}",
                              TEnumTraits<VulkGBufAttachment>::findName(attachment), (void*)vdi->view->image);
            }
        }
    };

   public:
    Vulk& vk;
    VkRenderPass renderPass;
    std::array<std::shared_ptr<VulkGBufs>, MAX_FRAMES_IN_FLIGHT> geoBufs;
    std::array<VkFramebuffer, MAX_FRAMES_IN_FLIGHT> frameBuffers;
    std::shared_ptr<VulkPipeline> deferredGeoPipeline;
    std::shared_ptr<VulkPipeline> deferredLightingPipeline;
    std::shared_ptr<VulkDescriptorSetInfo> deferredLightingDescriptorSetInfo;
    std::shared_ptr<VulkSampler> textureSampler;

    VulkDeferredRenderpass(Vulk& vkIn, VulkResources& resources, VulkScene* scene) : vk(vkIn) {
        // TODO: remove
        logger->set_level(spdlog::level::debug);
        for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
            geoBufs[i] = std::make_unique<VulkGBufs>(vk);
        }

        // the geo subpass needs attachments for each gbuf and the depth buffer that it writes to:
        // 1. a color attachment for each gbuf
        // 2. a depth attachment

        const uint32_t numAttachments = (uint32_t)geoBufs[0]->gbufs.size();
        std::vector<VkAttachmentDescription> geoAttachments(numAttachments);
        // std::vector<VulkGBufAttachment> geoAttachmentTypes = {VulkGBufAttachment::Albedo, VulkGBufAttachment::Normal,
        //                                                       VulkGBufAttachment::Depth,
        //                                                       VulkGBufAttachment::Material};
        // static_assert(TEnumTraits<VulkGBufAttachment>::max() == VulkGBufAttachment::Material);

        for (VulkGBufAttachment a : TEnumTraits<VulkGBufAttachment>::values) {
            bool isDepth = a == VulkGBufAttachment::Depth;
            size_t i = (size_t)a;
            geoAttachments[i].format = geoBufs[0]->gbufs.at(a)->format;
            geoAttachments[i].samples = VK_SAMPLE_COUNT_1_BIT;
            geoAttachments[i].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
            geoAttachments[i].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
            geoAttachments[i].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
            geoAttachments[i].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
            geoAttachments[i].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
            geoAttachments[i].finalLayout =
                isDepth ? VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL : VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        }

        std::vector<VkAttachmentReference> geoAttachmentRefs;
        VkAttachmentReference geoDepthAttachmentRef = {};
        for (uint32_t i = 0; i < numAttachments; i++) {
            if (i == (size_t)VulkGBufAttachment::Depth) {
                geoDepthAttachmentRef.attachment = i;
                geoDepthAttachmentRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
            } else {
                VkAttachmentReference colorAttachmentRef = {
                    .attachment = i,
                    .layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
                };
                geoAttachmentRefs.push_back(colorAttachmentRef);
            }
        }

        // make the geo subpass
        // it writes its data to the geo buffers in the color attachments and depth attachment
        VkSubpassDescription geoSubpassDescription = {};
        geoSubpassDescription.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
        geoSubpassDescription.colorAttachmentCount = (uint32_t)geoAttachmentRefs.size();
        geoSubpassDescription.pColorAttachments = geoAttachmentRefs.data();
        geoSubpassDescription.pDepthStencilAttachment = &geoDepthAttachmentRef;

        // make the lighting subpass
        // this takes the gbufs as input and writes to the vulk swapchain image view
        VkAttachmentDescription lightingColorAttachment{};
        lightingColorAttachment.format = vk.swapChainImageFormat;
        lightingColorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
        lightingColorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        lightingColorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        lightingColorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        lightingColorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        lightingColorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        lightingColorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

        VkAttachmentDescription lightingDepthAttachment{};
        lightingDepthAttachment.format = vk.findDepthFormat();
        lightingDepthAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
        lightingDepthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        lightingDepthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        lightingDepthAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        lightingDepthAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        lightingDepthAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        lightingDepthAttachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

        VkAttachmentReference lightingColorAttachmentRef{};
        lightingColorAttachmentRef.attachment = numAttachments;
        lightingColorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

        VkAttachmentReference lightingDepthAttachmentRef{};
        lightingDepthAttachmentRef.attachment = numAttachments + 1;
        lightingDepthAttachmentRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

        VkSubpassDescription lightingSubpassDescription{};
        lightingSubpassDescription.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
        lightingSubpassDescription.colorAttachmentCount = 1;
        lightingSubpassDescription.pColorAttachments = &lightingColorAttachmentRef;
        lightingSubpassDescription.pDepthStencilAttachment = &lightingDepthAttachmentRef;

        // this first dependency resets the gbufs to a state where they can be written to
        // the second dependency waits for the gbufs to be written and transitions them to a
        // state where they can be read
        VkSubpassDependency preGeoDependency = {};
        preGeoDependency.srcSubpass = VK_SUBPASS_EXTERNAL;  // External dependency
        preGeoDependency.dstSubpass = 0;                    // First subpass
        preGeoDependency.srcStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
        preGeoDependency.dstStageMask =
            VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
        preGeoDependency.srcAccessMask = VK_ACCESS_MEMORY_READ_BIT;
        preGeoDependency.dstAccessMask =
            VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
        preGeoDependency.dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

        VkSubpassDependency geoToLightDependency = {};
        geoToLightDependency.srcSubpass = 0;  // First subpass
        geoToLightDependency.dstSubpass = 1;  // Second subpass
        geoToLightDependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        geoToLightDependency.dstStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
        geoToLightDependency.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
        geoToLightDependency.dstAccessMask =
            VK_ACCESS_SHADER_READ_BIT;  // | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT;
        geoToLightDependency.dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

        std::vector<VkSubpassDependency> dependencies = {preGeoDependency, geoToLightDependency};

        std::vector<VkAttachmentDescription> attachments(geoAttachments);
        attachments.push_back(lightingColorAttachment);
        attachments.push_back(lightingDepthAttachment);

        VkRenderPassCreateInfo renderPassInfo = {};
        renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
        renderPassInfo.attachmentCount = (uint32_t)attachments.size();
        renderPassInfo.pAttachments = attachments.data();
        renderPassInfo.subpassCount = 2;
        VkSubpassDescription subpasses[] = {geoSubpassDescription, lightingSubpassDescription};
        renderPassInfo.pSubpasses = subpasses;
        renderPassInfo.dependencyCount = (uint32_t)dependencies.size();
        renderPassInfo.pDependencies = dependencies.data();
        VK_CALL(vkCreateRenderPass(vk.device, &renderPassInfo, nullptr, &renderPass));

        std::array<std::array<VkImageView, NumGeoBufs + 2>, MAX_FRAMES_IN_FLIGHT> framebufImageViews;
        for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
            std::copy(geoBufs[i]->gbufViews.begin(), geoBufs[i]->gbufViews.end(), framebufImageViews[i].begin());
            framebufImageViews[i][NumGeoBufs] = vk.swapChainImageViews[i];
            framebufImageViews[i][NumGeoBufs + 1] = vk.depthImageView;

            VkFramebufferCreateInfo framebufferInfo = {};
            framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
            framebufferInfo.renderPass = renderPass;
            framebufferInfo.attachmentCount = (uint32_t)framebufImageViews[i].size();
            framebufferInfo.pAttachments = framebufImageViews[i].data();
            framebufferInfo.width = vk.swapChainExtent.width;
            framebufferInfo.height = vk.swapChainExtent.height;
            framebufferInfo.layers = 1;

            VK_CALL(vkCreateFramebuffer(vk.device, &framebufferInfo, nullptr, &frameBuffers[i]));
        }

        deferredGeoPipeline = resources.loadPipeline(renderPass, vk.swapChainExtent, "DeferredRenderGeo");
        deferredLightingPipeline = resources.loadPipeline(renderPass, vk.swapChainExtent, "DeferredRenderLighting");

        textureSampler = VulkSampler::createImageSampler(vk);

        VulkDescriptorSetBuilder dsBuilder(vk);
        // this makes sue our .frag file matches what we do here
        dsBuilder.setDescriptorSetLayout(deferredLightingPipeline->descriptorSetLayout);
        dsBuilder.addFrameUBOs(scene->sceneUBOs.eyePos, VK_SHADER_STAGE_FRAGMENT_BIT, VulkShaderUBOBinding::EyePos);
        dsBuilder.addUniformBuffer(scene->sceneUBOs.pointLight, VK_SHADER_STAGE_FRAGMENT_BIT,
                                   VulkShaderUBOBinding::Lights);
        if (scene->pbrDebugUBO == nullptr)
            scene->pbrDebugUBO = std::make_shared<VulkUniformBuffer<VulkPBRDebugUBO>>(vk);
        dsBuilder.addUniformBuffer(*scene->pbrDebugUBO, VK_SHADER_STAGE_FRAGMENT_BIT,
                                   VulkShaderUBOBinding::PBRDebugUBO);
        if (scene->invViewProjUBO == nullptr)
            scene->invViewProjUBO = std::make_shared<VulkUniformBuffer<glm::mat4>>(vk);

        for (uint32_t frame = 0; frame < MAX_FRAMES_IN_FLIGHT; frame++) {
            auto geoBuf = geoBufs[frame];
            dsBuilder.addFrameImageSampler(frame, VK_SHADER_STAGE_FRAGMENT_BIT, VulkShaderTextureBinding::GBufAlbedo,
                                           geoBuf->gbufs.at(VulkGBufAttachment::Albedo)->view, textureSampler);
            dsBuilder.addFrameImageSampler(frame, VK_SHADER_STAGE_FRAGMENT_BIT, VulkShaderTextureBinding::GBufNormal,
                                           geoBuf->gbufs.at(VulkGBufAttachment::Normal)->view, textureSampler);
            dsBuilder.addFrameImageSampler(frame, VK_SHADER_STAGE_FRAGMENT_BIT, VulkShaderTextureBinding::GBufDepth,
                                           geoBuf->gbufs.at(VulkGBufAttachment::Depth)->view, textureSampler);
            dsBuilder.addFrameImageSampler(frame, VK_SHADER_STAGE_FRAGMENT_BIT, VulkShaderTextureBinding::GBufMaterial,
                                           geoBuf->gbufs.at(VulkGBufAttachment::Material)->view, textureSampler);
        }
        deferredLightingDescriptorSetInfo = dsBuilder.build();
    }

    void beginRenderToGBufs(VkCommandBuffer commandBuffer) {
        VkRenderPassBeginInfo renderPassInfo{};
        renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
        renderPassInfo.renderPass = renderPass;
        renderPassInfo.framebuffer = frameBuffers[vk.currentFrame];
        renderPassInfo.renderArea.offset = {0, 0};
        renderPassInfo.renderArea.extent = vk.swapChainExtent;

        std::array<VkClearValue, 7> clearValues{};
        clearValues[(int)VulkGBufAttachment::Depth].depthStencil = {1.0f, 0};

        renderPassInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
        renderPassInfo.pClearValues = clearValues.data();

        vkCmdBeginRenderPass(commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
    }

    void renderGBufsAndEnd(VkCommandBuffer commandBuffer) {
        vkCmdNextSubpass(commandBuffer, VK_SUBPASS_CONTENTS_INLINE);
        vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, deferredLightingPipeline->pipeline);
        vkCmdBindDescriptorSets(
            commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, deferredLightingPipeline->pipelineLayout, 0, 1,
            &deferredLightingDescriptorSetInfo->descriptorSets[vk.currentFrame]->descriptorSet, 0, nullptr);
        vkCmdDraw(commandBuffer, 4, 1, 0, 0);  // the vert shader handles this, just need 4 verts to draw a quad
        vkCmdEndRenderPass(commandBuffer);
    }

    ~VulkDeferredRenderpass() {
        for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
            vkDestroyFramebuffer(vk.device, frameBuffers[i], nullptr);
        }
        vkDestroyRenderPass(vk.device, renderPass, nullptr);
    }
};

}  // namespace vulk