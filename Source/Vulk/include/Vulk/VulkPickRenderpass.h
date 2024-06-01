#pragma once
#include <vulkan/vulkan.h>

#include "ClassNonCopyableNonMovable.h"

class Vulk;

class VulkPickView : public ClassNonCopyableNonMovable {
public:
    Vulk& vk;
    std::shared_ptr<VulkImageView> view;
    VkExtent2D extent;
    VkFormat format;

    VulkPickView(Vulk& vkIn, VkExtent2D extentIn, VkFormat formatIn)
        : vk(vkIn)
        , extent(extentIn)
        , format(formatIn) {
        VkImage image;
        VkDeviceMemory imageMemory;
        VkImageView pickImageView;

        vk.createImage(extent.width, extent.height, format, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT,
                       VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, image, imageMemory);
        pickImageView = vk.createImageView(image, format, VK_IMAGE_ASPECT_COLOR_BIT);
        view = std::make_shared<VulkImageView>(vk, image, imageMemory, pickImageView);
    }
};

// A renderpass for rendering objectids to so you can see what
// the mouse currently has selected.
class VulkPickRenderpass : public ClassNonCopyableNonMovable {
    std::shared_ptr<VulkFence> pickFence;

public:
    Vulk& vk;
    VkRenderPass renderPass;
    std::array<std::shared_ptr<VulkPickView>, MAX_FRAMES_IN_FLIGHT> pickViews;
    std::array<VkFramebuffer, MAX_FRAMES_IN_FLIGHT> frameBuffers;
    VkExtent2D extent = {};
    VkFormat format = VK_FORMAT_R32_UINT;
    std::vector<uint32_t> pickData;

    VulkPickRenderpass(Vulk& vkIn)
        : vk(vkIn) {
        pickFence = std::make_shared<VulkFence>(vk);

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
        attachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;          // Initial layout of the attachment before the render pass starts
        attachment.finalLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL; // put it in a format we can read into CPU memory

        VkAttachmentReference colorAttachmentRef = {};
        colorAttachmentRef.attachment = 0;                                    // The index of the attachment in the attachment description array
        colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL; // Layout during the subpass

        VkSubpassDescription subpassDescription = {};
        subpassDescription.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
        subpassDescription.colorAttachmentCount = 1;                // Number of color attachments
        subpassDescription.pColorAttachments = &colorAttachmentRef; // Array of color attachments

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

        pickData.resize(extent.width * extent.height);
    }

    void updatePickDataFromBuffer(uint32_t frameIndex) {
        // use a fence to wait for the pick buffer to be done
        VK_CALL(vkWaitForFences(vk.device, 1, &pickFence->fence, VK_TRUE, UINT64_MAX));
        VulkPickView* pickView = pickViews[frameIndex].get();
        vk.copyImageToMem(pickView->view->image, pickData.data(), pickData.size() * sizeof(uint32_t));
    }

    ~VulkPickRenderpass() {
        for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
            vkDestroyFramebuffer(vk.device, frameBuffers[i], nullptr);
        }
        vkDestroyRenderPass(vk.device, renderPass, nullptr);
    }
};
