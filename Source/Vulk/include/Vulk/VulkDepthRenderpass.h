#pragma once
#include <vulkan/vulkan.h>

#include "Common/ClassNonCopyableNonMovable.h"
#include "VulkDepthView.h"

class Vulk;

// A renderpass for filling a depth buffer.
// e.g. For shadow mapping, you generally need a render pass with a single depth attachment,
// since you're interested in capturing depth information from the light's perspective.
class VulkDepthRenderpass : public ClassNonCopyableNonMovable {
  public:
    Vulk &vk;
    VkRenderPass renderPass;
    std::array<std::shared_ptr<VulkDepthView>, MAX_FRAMES_IN_FLIGHT> depthViews;
    std::array<VkFramebuffer, MAX_FRAMES_IN_FLIGHT> frameBuffers;
    VkExtent2D extent = {1024, 1024};
    VkFormat depthFormat = VK_FORMAT_D32_SFLOAT;

    VulkDepthRenderpass(Vulk &vkIn) : vk(vkIn) {
        for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
            depthViews[i] = std::make_unique<VulkDepthView>(vk, extent, depthFormat);
        }

        VkAttachmentDescription depthAttachment = {};
        depthAttachment.format = depthFormat;
        depthAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
        depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;   // Clear depth at the start of the render pass
        depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE; // Store the depth information after rendering
        depthAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        depthAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        depthAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;                      // Initial layout of the attachment before the render pass starts
        depthAttachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL; // Layout to automatically transition to after the render pass

        VkAttachmentReference depthAttachmentRef = {};
        depthAttachmentRef.attachment = 0; // The index of the depth attachment in the attachment description array
        depthAttachmentRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

        VkSubpassDescription subpassDescription = {};
        subpassDescription.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
        subpassDescription.pDepthStencilAttachment = &depthAttachmentRef;

        VkRenderPassCreateInfo renderPassInfo = {};
        renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
        renderPassInfo.attachmentCount = 1;
        renderPassInfo.pAttachments = &depthAttachment;
        renderPassInfo.subpassCount = 1;
        renderPassInfo.pSubpasses = &subpassDescription;

        VK_CALL(vkCreateRenderPass(vk.device, &renderPassInfo, nullptr, &renderPass));

        for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
            VkFramebufferCreateInfo framebufferInfo = {};
            framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
            framebufferInfo.renderPass = renderPass;
            framebufferInfo.attachmentCount = 1;
            framebufferInfo.pAttachments = &depthViews[i]->depthView->imageView;
            framebufferInfo.width = extent.width;
            framebufferInfo.height = extent.height;
            framebufferInfo.layers = 1;

            VK_CALL(vkCreateFramebuffer(vk.device, &framebufferInfo, nullptr, &frameBuffers[i]));
        }
    }
    ~VulkDepthRenderpass() {
        for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
            vkDestroyFramebuffer(vk.device, frameBuffers[i], nullptr);
        }
        vkDestroyRenderPass(vk.device, renderPass, nullptr);
    }

  private:
    void loadTextureView(char const *texturePath, bool isUNORM);
};
