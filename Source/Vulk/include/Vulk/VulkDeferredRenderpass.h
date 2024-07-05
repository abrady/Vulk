#pragma once

#include <vulkan/vulkan.h>

#include "Vulk/ClassNonCopyableNonMovable.h"
#include "Vulk/Vulk.h"
#include "Vulk/VulkImageView.h"

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
    using VulkGeoBufAttachment = vulk::cpp2::VulkGeoBufAttachment;

   public:
    Vulk& vk;
    std::unordered_map<VulkGeoBufAttachment, std::shared_ptr<VulkDeferredImage>> gbufs;
    std::array<VkImageView, apache::thrift::TEnumTraits<VulkGeoBufAttachment>::size> gbufViews;

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

class VulkDeferredRenderpass : public ClassNonCopyableNonMovable {
    using VulkGeoBufAttachment = vulk::cpp2::VulkGeoBufAttachment;

   public:
    Vulk& vk;
    VkRenderPass renderPass;
    std::array<std::shared_ptr<VulkGBufs>, MAX_FRAMES_IN_FLIGHT> geoBufs;
    std::array<VkFramebuffer, MAX_FRAMES_IN_FLIGHT> frameBuffers;
    VkFormat format = VK_FORMAT_R32_UINT;

    VulkDeferredRenderpass(Vulk& vkIn) : vk(vkIn) {
        for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
            geoBufs[i] = std::make_unique<VulkGBufs>(vk);
        }

        const uint32_t numAttachments = (uint32_t)geoBufs[0]->gbufs.size();
        std::vector<VkAttachmentDescription> attachments(numAttachments);
        for (uint32_t i = 0; i < numAttachments; i++) {
            bool isDepth = i == (uint32_t)VulkGeoBufAttachment::Depth;
            attachments[i].format = geoBufs[0]->gbufs.at(static_cast<VulkGeoBufAttachment>(i))->format;
            attachments[i].samples = VK_SAMPLE_COUNT_1_BIT;
            attachments[i].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
            attachments[i].storeOp = isDepth ? VK_ATTACHMENT_STORE_OP_DONT_CARE : VK_ATTACHMENT_STORE_OP_STORE;
            attachments[i].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
            attachments[i].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
            attachments[i].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
            attachments[i].finalLayout =
                isDepth ? VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL : VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
        }

        std::vector<VkAttachmentReference> colorAttachmentRefs;
        VkAttachmentReference depthAttachmentRef = {};
        for (uint32_t i = 0; i < numAttachments; i++) {
            if (i == (size_t)VulkGeoBufAttachment::Depth) {
                depthAttachmentRef.attachment = i;
                depthAttachmentRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
            } else {
                VkAttachmentReference colorAttachmentRef = {
                    .attachment = i,
                    .layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
                };
                colorAttachmentRefs.push_back(colorAttachmentRef);
            }
        }

        // make the geo subpass
        VkSubpassDescription geoSubpassDescription = {};
        geoSubpassDescription.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
        geoSubpassDescription.colorAttachmentCount = (uint32_t)colorAttachmentRefs.size();
        geoSubpassDescription.pColorAttachments = colorAttachmentRefs.data();
        geoSubpassDescription.pDepthStencilAttachment = &depthAttachmentRef;

        // make the lighting subpass
        VkSubpassDescription lightingSubpassDescription = {};
        lightingSubpassDescription.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
        lightingSubpassDescription.colorAttachmentCount = (uint32_t)colorAttachmentRefs.size();
        lightingSubpassDescription.pColorAttachments = colorAttachmentRefs.data();
        lightingSubpassDescription.pDepthStencilAttachment = &depthAttachmentRef;

        VkRenderPassCreateInfo renderPassInfo = {};
        renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
        renderPassInfo.attachmentCount = numAttachments;
        renderPassInfo.pAttachments = attachments.data();
        renderPassInfo.subpassCount = 2;
        VkSubpassDescription subpasses[] = {geoSubpassDescription, lightingSubpassDescription};
        renderPassInfo.pSubpasses = subpasses;
        VK_CALL(vkCreateRenderPass(vk.device, &renderPassInfo, nullptr, &renderPass));

        for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
            VkFramebufferCreateInfo framebufferInfo = {};
            framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
            framebufferInfo.renderPass = renderPass;
            framebufferInfo.attachmentCount = numAttachments;
            framebufferInfo.pAttachments = geoBufs[i]->gbufViews.data();
            framebufferInfo.width = vk.swapChainExtent.width;
            framebufferInfo.height = vk.swapChainExtent.height;
            framebufferInfo.layers = 1;

            VK_CALL(vkCreateFramebuffer(vk.device, &framebufferInfo, nullptr, &frameBuffers[i]));
        }
    }

    ~VulkDeferredRenderpass() {
        for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
            vkDestroyFramebuffer(vk.device, frameBuffers[i], nullptr);
        }
        vkDestroyRenderPass(vk.device, renderPass, nullptr);
    }
};