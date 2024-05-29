#pragma once
#include <vulkan/vulkan.h>

#include "ClassNonCopyableNonMovable.h"
#include "VulkPickView.h"

class Vulk;

class VulkPickView : public ClassNonCopyableNonMovable {
public:
    Vulk& vk;
    std::shared_ptr<VulkTextureView> texView;
    VkExtent2D extent;
    VkFormat format;

    VulkPickView(Vulk& vkIn, VkExtent2D extentIn, VkFormat formatIn)
        : vk(vkIn)
        , extent(extentIn)
        , format(formatIn) {
        VkImage image;
        VkDeviceMemory imageMemory;
        VkImageView pickImageView;

        vk.createImage(extent.width, extent.height, format, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE__STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
                       VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, image, imageMemory);
        pickImageView = vk.createImageView(image, format, VK_IMAGE_ASPECT__BIT);
        texView = std::make_shared<VulkTextureView>(vk, image, imageMemory, pickImageView);
    }
};

// A renderpass for rendering objectids to so you can see what
// the mouse currently has selected.
class VulkPickRenderpass : public ClassNonCopyableNonMovable {
public:
    Vulk& vk;
    VkRenderPass renderPass;
    std::array<std::shared_ptr<VulkPickView>, MAX_FRAMES_IN_FLIGHT> pickViews;
    std::array<VkFramebuffer, MAX_FRAMES_IN_FLIGHT> frameBuffers;
    VkExtent2D extent = {};
    VkFormat format = VK_FORMAT_R32_UINT;

    VulkPickRenderpass(Vulk& vkIn)
        : vk(vkIn) {
        // I've been told matching the aspect ratio is important for shadow mapping
        extent.width = vk.swapChainExtent.width;
        extent.height = vk.swapChainExtent.height;
        for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
            pickViews[i] = std::make_unique<VulkPickView>(vk, extent, format);
        }

        VkAttachmentDescription attachment = {};
        attachment.format = format;
        attachment.samples = VK_SAMPLE_COUNT_1_BIT;
        attachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;   // Clear  at the start of the render pass
        attachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE; // Store the  information after rendering
        attachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        attachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;                 // Initial layout of the attachment before the render pass starts
        attachment.finalLayout = VK_IMAGE_LAYOUT__STENCIL_ATTACHMENT_OPTIMAL; // Layout to automatically transition to after the render pass

        VkAttachmentReference attachmentRef = {};
        attachmentRef.attachment = 0; // The index of the  attachment in the attachment description array
        attachmentRef.layout = VK_IMAGE_LAYOUT__STENCIL_ATTACHMENT_OPTIMAL;

        VkSubpassDescription subpassDescription = {};
        subpassDescription.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
        subpassDescription.pstencilAttachment = &attachmentRef;

        VkRenderPassCreateInfo renderPassInfo = {};
        renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
        renderPassInfo.attachmentCount = 1;
        renderPassInfo.pAttachments = &attachment;
        renderPassInfo.subpassCount = 1;
        renderPassInfo.pSubpasses = &subpassDescription;

        VK_CALL(vkCreateRenderPass(vk.device, &renderPassInfo, nullptr, &renderPass));

        for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
            VkFramebufferCreateInfo framebufferInfo = {};
            framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
            framebufferInfo.renderPass = renderPass;
            framebufferInfo.attachmentCount = 1;
            framebufferInfo.pAttachments = &pickViews[i]->view->imageView;
            framebufferInfo.width = extent.width;
            framebufferInfo.height = extent.height;
            framebufferInfo.layers = 1;

            VK_CALL(vkCreateFramebuffer(vk.device, &framebufferInfo, nullptr, &frameBuffers[i]));
        }
    }
    ~VulkPickRenderpass() {
        for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
            vkDestroyFramebuffer(vk.device, frameBuffers[i], nullptr);
        }
        vkDestroyRenderPass(vk.device, renderPass, nullptr);
    }
};
