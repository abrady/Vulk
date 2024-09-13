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

class VulkResources;
class VulkScene;

namespace vulk {

class VulkDeferredRenderpass : public ClassNonCopyableNonMovable {
    inline static std::shared_ptr<spdlog::logger> logger = VulkLogger::CreateLogger("VulkDeferredRenderpass");

    using GBufAtmtIdx              = vulk::cpp2::GBufAtmtIdx;
    using GBufBinding              = vulk::cpp2::GBufBinding;
    using GBufInputAtmtIdx         = vulk::cpp2::GBufInputAtmtIdx;
    using VulkShaderTextureBinding = vulk::cpp2::VulkShaderTextureBinding;
    using VulkShaderUBOBinding     = vulk::cpp2::VulkShaderUBOBinding;

   public:
    class DeferredImage : public ClassNonCopyableNonMovable {
       public:
        Vulk& vk;
        std::shared_ptr<VulkImageView> view;
        VkFormat format;

        DeferredImage(Vulk& vkIn, VkFormat formatIn, bool isDepth) : vk(vkIn), format(formatIn) {
            VkImage image;
            VkDeviceMemory imageMemory;
            VkImageView imageView;

            // VK_IMAGE_USAGE_SAMPLED_BIT so that we can sample from the image in the shader
            if (isDepth) {
                vk.createImage(vk.swapChainExtent.width,
                               vk.swapChainExtent.height,
                               format,
                               VK_IMAGE_TILING_OPTIMAL,
                               VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
                               VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                               image,
                               imageMemory);
                imageView = vk.createImageView(image, format, VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT);
            } else {
                vk.createImage(
                    vk.swapChainExtent.width,
                    vk.swapChainExtent.height,
                    format,
                    VK_IMAGE_TILING_OPTIMAL,
                    VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT,
                    VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                    image,
                    imageMemory);
                imageView = vk.createImageView(image, format, VK_IMAGE_ASPECT_COLOR_BIT);
            }
            view = std::make_shared<VulkImageView>(vk, image, imageMemory, imageView);
        }
    };

    class VulkGBufs : public ClassNonCopyableNonMovable {
       public:
        Vulk& vk;
        std::unordered_map<GBufAtmtIdx, std::unique_ptr<DeferredImage>> gbufs;
        std::array<VkImageView, TEnumTraits<vulk::cpp2::GBufAtmtIdx>::size> gbufViews;

        DeferredImage* imageFromInput(GBufInputAtmtIdx input) {
            GBufAtmtIdx atmt;
            switch (input) {
                case GBufInputAtmtIdx::Albedo:
                    atmt = GBufAtmtIdx::Albedo;
                    break;
                case GBufInputAtmtIdx::Normal:
                    atmt = GBufAtmtIdx::Normal;
                    break;
                case GBufInputAtmtIdx::Material:
                    atmt = GBufAtmtIdx::Material;
                    break;
                default:
                    VULK_THROW("Invalid GBufInputAtmtIdx {}", TEnumTraits<GBufInputAtmtIdx>::findName(input));
            }
            static_assert(TEnumTraits<GBufInputAtmtIdx>::size == 3);
            return gbufs.at(atmt).get();
        }

        DeferredImage* imageFromBinding(vulk::cpp2::GBufBinding binding) {
            GBufAtmtIdx atmt;
            switch (binding) {
                case GBufBinding::Normal:
                    atmt = GBufAtmtIdx::Normal;
                    break;
                case GBufBinding::Depth:
                    atmt = GBufAtmtIdx::Depth;
                    break;
                case GBufBinding::Albedo:
                    atmt = GBufAtmtIdx::Albedo;
                    break;
                case GBufBinding::Material:
                    atmt = GBufAtmtIdx::Material;
                    break;
                default:
                    VULK_THROW("Invalid GBufBinding {}", TEnumTraits<GBufBinding>::findName(binding));
            }
            static_assert(TEnumTraits<GBufAtmtIdx>::size == 5);
            return gbufs.at(atmt).get();
        }

        // normal could also be: VK_FORMAT_R16G16B16A16_SFLOAT
        // depth could be: VK_FORMAT_D24_UNORM_S8_UINT - could pack data in the stencil buffer
        // material could be more compact: VK_FORMAT_R4G4B4A4_UNORM or VK_FORMAT_R5G5B5A1_UNORM

        VulkGBufs(Vulk& vkIn) : vk(vkIn) {
            // GBufAtmtIdx::Color is excluded - that comes from the swapchain
            std::pair<GBufAtmtIdx, VkFormat> bindings[] = {
                {GBufAtmtIdx::Normal, VK_FORMAT_R16G16_SFLOAT},  // hemioct-encoded normal
                {GBufAtmtIdx::Depth, vk.findDepthFormat()},
                {GBufAtmtIdx::Albedo, VK_FORMAT_R8G8B8A8_UNORM},
                {GBufAtmtIdx::Material, VK_FORMAT_R8G8B8A8_UNORM},
            };
            for (auto& [attachment, format] : bindings) {
                bool isDepth               = attachment == GBufAtmtIdx::Depth;
                gbufs[attachment]          = std::make_unique<DeferredImage>(vk, format, isDepth);
                gbufViews[(int)attachment] = gbufs[attachment]->view->imageView;
                logger->debug("Created GBuf attachment: {} : image {}",
                              TEnumTraits<GBufAtmtIdx>::findName(attachment),
                              (void*)gbufs[attachment]->view->image);
            }
        }
    };

    Vulk& vk;
    VkRenderPass renderPass;
    std::unique_ptr<VulkGBufs> geoBufs;
    std::array<VkFramebuffer, MAX_FRAMES_IN_FLIGHT> frameBuffers;
    std::shared_ptr<VulkPipeline> deferredGeoPipeline;
    std::shared_ptr<VulkPipeline> deferredLightingPipeline;
    std::shared_ptr<VulkDescriptorSetInfo> deferredLightingDescriptorSetInfo;
    std::shared_ptr<VulkSampler> textureSampler;

    VulkDeferredRenderpass(Vulk& vkIn, VulkResources& resources, VulkScene& scene);

    void beginRenderToGBufs(VkCommandBuffer commandBuffer) {
        vk.beginDebugLabel(commandBuffer, "Deferred GBuffer Creation");
        std::array<VkClearValue, TEnumTraits<GBufAtmtIdx>::size + 1> clearValues{};
        clearValues[(int)GBufAtmtIdx::Depth].depthStencil = {1.0f, 0};

        VkRenderPassBeginInfo renderPassInfo{.sType       = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
                                             .renderPass  = renderPass,
                                             .framebuffer = frameBuffers[vk.currentFrame],
                                             .renderArea =
                                                 {
                                                     .offset = {0, 0},
                                                     .extent = vk.swapChainExtent,
                                                 },
                                             .clearValueCount = static_cast<uint32_t>(clearValues.size()),
                                             .pClearValues    = clearValues.data()};

        vkCmdBeginRenderPass(commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
        VkViewport viewport{
            .x        = 0.0f,
            .y        = 0.0f,
            .width    = (float)vk.swapChainExtent.width,
            .height   = (float)vk.swapChainExtent.height,
            .minDepth = 0.0f,
            .maxDepth = 1.0f,
        };

        vkCmdSetViewport(commandBuffer, 0, 1, &viewport);

        VkRect2D scissor{
            .offset = {0, 0},
            .extent = vk.swapChainExtent,
        };
        vkCmdSetScissor(commandBuffer, 0, 1, &scissor);
    }

    void renderGBufsAndEnd(VkCommandBuffer commandBuffer) {
        vk.endDebugLabel(commandBuffer);
        vk.beginDebugLabel(commandBuffer, "Deferred Lighting Pass");
        vkCmdNextSubpass(commandBuffer, VK_SUBPASS_CONTENTS_INLINE);
        vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, deferredLightingPipeline->pipeline);
        vkCmdBindDescriptorSets(commandBuffer,
                                VK_PIPELINE_BIND_POINT_GRAPHICS,
                                deferredLightingPipeline->pipelineLayout,
                                0,
                                1,
                                &deferredLightingDescriptorSetInfo->descriptorSets[vk.currentFrame]->descriptorSet,
                                0,
                                nullptr);
        vkCmdDraw(commandBuffer, 4, 1, 0, 0);  // the vert shader handles this, just need 4 verts to draw a quad
        vkCmdEndRenderPass(commandBuffer);
        vk.endDebugLabel(commandBuffer);
    }

    ~VulkDeferredRenderpass() {
        for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
            vkDestroyFramebuffer(vk.device, frameBuffers[i], nullptr);
        }
        vkDestroyRenderPass(vk.device, renderPass, nullptr);
    }
};  // namespace vulk

}  // namespace vulk