#pragma once

#include <vulkan/vulkan.h>

#include "Vulk/ClassNonCopyableNonMovable.h"
#include "Vulk/Vulk.h"
#include "Vulk/VulkDescriptorSetLayout.h"
#include "Vulk/VulkImageView.h"
#include "Vulk/VulkPipeline.h"
#include "Vulk/VulkResources.h"

namespace vulk {

constexpr uint32_t NumGeoBufs = apache::thrift::TEnumTraits<vulk::cpp2::VulkGeoBufAttachment>::size;
class VulkDeferredRenderpass : public ClassNonCopyableNonMovable {
    using VulkGeoBufAttachment = vulk::cpp2::VulkGeoBufAttachment;

    class VulkDeferredImage : public ClassNonCopyableNonMovable {
       public:
        Vulk& vk;
        std::shared_ptr<VulkImageView> view;
        VkFormat format;

        VulkDeferredImage(Vulk& vkIn, VkFormat formatIn, bool isDepth) : vk(vkIn), format(formatIn) {
            VkImage image;
            VkDeviceMemory imageMemory;
            VkImageView imageView;

            if (isDepth) {
                vk.createImage(vk.swapChainExtent.width, vk.swapChainExtent.height, format, VK_IMAGE_TILING_OPTIMAL,
                               VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, image,
                               imageMemory);
                imageView = vk.createImageView(image, format, VK_IMAGE_ASPECT_DEPTH_BIT);
            } else {
                vk.createImage(vk.swapChainExtent.width, vk.swapChainExtent.height, format, VK_IMAGE_TILING_OPTIMAL,
                               VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, image,
                               imageMemory);
                imageView = vk.createImageView(image, format, VK_IMAGE_ASPECT_COLOR_BIT);
            }
            view = std::make_shared<VulkImageView>(vk, image, imageMemory, imageView);
        }
    };

    class VulkGBufs : public ClassNonCopyableNonMovable {
       public:
        Vulk& vk;
        std::unordered_map<VulkGeoBufAttachment, std::shared_ptr<VulkDeferredImage>> gbufs;
        std::array<VkImageView, NumGeoBufs> gbufViews;

        VulkGBufs(Vulk& vkIn) : vk(vkIn) {
            std::pair<VulkGeoBufAttachment, VkFormat> bindings[] = {
                {VulkGeoBufAttachment::Normal, VK_FORMAT_R16G16B16A16_SFLOAT},
                {VulkGeoBufAttachment::Depth, vk.findDepthFormat()},
                {VulkGeoBufAttachment::Albedo, VK_FORMAT_R8G8B8A8_UNORM},
                {VulkGeoBufAttachment::Material, VK_FORMAT_R8G8B8A8_UNORM},
                {VulkGeoBufAttachment::Specular, VK_FORMAT_R8G8B8A8_UNORM},
            };
            for (auto& binding : bindings) {
                bool isDepth = binding.first == VulkGeoBufAttachment::Depth;
                auto vdi = std::make_shared<VulkDeferredImage>(vk, binding.second, isDepth);
                gbufs[binding.first] = vdi;
                gbufViews[(int)binding.first] = vdi->view->imageView;
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

    VulkDeferredRenderpass(Vulk& vkIn, VulkResources& resources) : vk(vkIn) {
        for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
            geoBufs[i] = std::make_unique<VulkGBufs>(vk);
        }

        // the geo subpass needs attachments for each gbuf and the depth buffer that it writes to:
        // 1. a color attachment for each gbuf
        // 2. a depth attachment

        const uint32_t numAttachments = (uint32_t)geoBufs[0]->gbufs.size();
        std::vector<VkAttachmentDescription> geoAttachments(numAttachments);
        for (uint32_t i = 0; i < numAttachments; i++) {
            bool isDepth = i == (uint32_t)VulkGeoBufAttachment::Depth;
            geoAttachments[i].format = geoBufs[0]->gbufs.at(static_cast<VulkGeoBufAttachment>(i))->format;
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
            if (i == (size_t)VulkGeoBufAttachment::Depth) {
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

        VkSubpassDependency dependency = {};
        dependency.srcSubpass = 0;  // First subpass
        dependency.dstSubpass = 1;  // Second subpass
        dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        dependency.dstStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
        dependency.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
        dependency.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
        dependency.dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

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
        renderPassInfo.dependencyCount = 1;
        renderPassInfo.pDependencies = &dependency;
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
    }

    void beginRenderPass(VkCommandBuffer commandBuffer) {
        VkRenderPassBeginInfo renderPassInfo{};
        renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
        renderPassInfo.renderPass = renderPass;
        renderPassInfo.framebuffer = frameBuffers[vk.currentFrame];
        renderPassInfo.renderArea.offset = {0, 0};
        renderPassInfo.renderArea.extent = vk.swapChainExtent;

        std::array<VkClearValue, 5> clearValues{};
        clearValues[(int)VulkGeoBufAttachment::Depth].depthStencil = {1.0f, 0};

        renderPassInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
        renderPassInfo.pClearValues = clearValues.data();

        vkCmdBeginRenderPass(commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
    }

    ~VulkDeferredRenderpass() {
        for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
            vkDestroyFramebuffer(vk.device, frameBuffers[i], nullptr);
        }
        vkDestroyRenderPass(vk.device, renderPass, nullptr);
    }
};

}  // namespace vulk