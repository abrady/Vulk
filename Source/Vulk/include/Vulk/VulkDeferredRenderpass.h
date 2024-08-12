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
                vk.createImage(
                    vk.swapChainExtent.width, vk.swapChainExtent.height, format, VK_IMAGE_TILING_OPTIMAL,
                    VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                    image, imageMemory
                );
                imageView = vk.createImageView(image, format, VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT);
            } else {
                vk.createImage(
                    vk.swapChainExtent.width, vk.swapChainExtent.height, format, VK_IMAGE_TILING_OPTIMAL,
                    VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, image,
                    imageMemory
                );
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
                logger->debug(
                    "Created GBuf attachment: {} : image {}", TEnumTraits<VulkGBufAttachment>::findName(attachment),
                    (void*)vdi->view->image
                );
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

        // We need the gbufs and the swapchain image as attachments to the renderpass
        // --------------------------------------------------------------------------

        std::vector<VkAttachmentDescription> attachments((uint32_t)geoBufs[0]->gbufs.size());
        for (VulkGBufAttachment a : TEnumTraits<VulkGBufAttachment>::values) {
            bool isDepth = a == VulkGBufAttachment::Depth;
            size_t i = (size_t)a;
            attachments[i].format = geoBufs[0]->gbufs.at(a)->format;
            attachments[i].samples = VK_SAMPLE_COUNT_1_BIT;
            attachments[i].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
            attachments[i].storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;  // VK_ATTACHMENT_STORE_OP_STORE;
            attachments[i].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
            attachments[i].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
            attachments[i].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
            // these aren't actually read from the shaders as textures, so we don't need to make them shader read only
            attachments[i].finalLayout =
                isDepth ? VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL : VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
            // isDepth ? VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL : VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        }

        // we also need the swapchain image
        VkAttachmentDescription colorAttachment{
            .format = vk.swapChainImageFormat,
            .samples = VK_SAMPLE_COUNT_1_BIT,
            .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
            .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
            .stencilLoadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
            .stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
            .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
            .finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
        };
        attachments.push_back(colorAttachment);

        // used in both subpasses
        VkAttachmentReference depthRef = {
            .attachment = (uint32_t)VulkGBufAttachment::Depth,
            .layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
        };

        // ------------------------------ First Subpass: Geometry Pass ------------------------------

        std::vector<VkAttachmentReference> geoAttachmentRefs;
        for (uint32_t i = 0; i < attachments.size(); i++) {
            if (i == (uint32_t)VulkGBufAttachment::Depth) {
                continue;
            }
            VkAttachmentReference colorAttachmentRef = {
                .attachment = i,
                .layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
            };
            geoAttachmentRefs.push_back(colorAttachmentRef);
        }

        // make the geo subpass
        // it writes its data to the geo buffers in the color attachments and depth attachment
        VkSubpassDescription geoSubpassDescription = {
            .pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS,
            .colorAttachmentCount = (uint32_t)geoAttachmentRefs.size(),
            .pColorAttachments = geoAttachmentRefs.data(),
            .pDepthStencilAttachment = &depthRef,
        };

        // ------------------------------ Second Subpass: Lighting Pass ------------------------------

        VkAttachmentReference lightingColorRef{
            .attachment = (uint32_t)attachments.size() - 1,  // the swapchain image is the last attachment
            .layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
        };

        std::vector<VkAttachmentReference> inputReferences;
        for (VulkGBufAttachment a : TEnumTraits<VulkGBufAttachment>::values) {
            if (a == VulkGBufAttachment::Depth)
                continue;
            VkAttachmentReference inputRef{
                .attachment = (uint32_t)a,
                .layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
            };
            inputReferences.push_back(inputRef);
        }

        VkSubpassDescription lightingSubpassDescription{
            .pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS,
            .inputAttachmentCount = (uint32_t)inputReferences.size(),
            .pInputAttachments = inputReferences.data(),
            .colorAttachmentCount = 1,
            .pColorAttachments = &lightingColorRef,
            .pDepthStencilAttachment = &depthRef,
        };

        // ------------------------------ Subpass Dependencies ------------------------------

        std::array<VkSubpassDependency, 4> dependencies;
        // This makes sure that writes to the depth image are done before we try to write to it again
        dependencies[0].srcSubpass = VK_SUBPASS_EXTERNAL;
        dependencies[0].dstSubpass = 0;
        dependencies[0].srcStageMask = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
        dependencies[0].dstStageMask = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
        dependencies[0].srcAccessMask = 0;
        dependencies[0].dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
        dependencies[0].dependencyFlags = 0;

        dependencies[1].srcSubpass = VK_SUBPASS_EXTERNAL;
        dependencies[1].dstSubpass = 0;
        dependencies[1].srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        dependencies[1].dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        dependencies[1].srcAccessMask = 0;
        dependencies[1].dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
        dependencies[1].dependencyFlags = 0;

        // This dependency transitions the input attachment from color attachment to input attachment read
        dependencies[2].srcSubpass = 0;
        dependencies[2].dstSubpass = 1;
        dependencies[2].srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        dependencies[2].dstStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
        dependencies[2].srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
        dependencies[2].dstAccessMask = VK_ACCESS_INPUT_ATTACHMENT_READ_BIT;
        dependencies[2].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

        // for when we add a transparent subpass
        // dependencies[3].srcSubpass = 1;
        // dependencies[3].dstSubpass = 2;
        // dependencies[3].srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        // dependencies[3].dstStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
        // dependencies[3].srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
        // dependencies[3].dstAccessMask = VK_ACCESS_INPUT_ATTACHMENT_READ_BIT;
        // dependencies[3].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

        dependencies[3].srcSubpass = 1;
        dependencies[3].dstSubpass = VK_SUBPASS_EXTERNAL;
        dependencies[3].srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        dependencies[3].dstStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
        dependencies[3].srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
        dependencies[3].dstAccessMask = VK_ACCESS_MEMORY_READ_BIT;
        dependencies[3].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

        // ------------------------------ Create Render Pass ------------------------------
        std::array<VkSubpassDescription, 2> subpassDescriptions{geoSubpassDescription, lightingSubpassDescription};

        VkRenderPassCreateInfo renderPassInfo = {
            .sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,
            .attachmentCount = static_cast<uint32_t>(attachments.size()),
            .pAttachments = attachments.data(),
            .subpassCount = static_cast<uint32_t>(subpassDescriptions.size()),
            .pSubpasses = subpassDescriptions.data(),
            .dependencyCount = static_cast<uint32_t>(dependencies.size()),
            .pDependencies = dependencies.data(),
        };
        VK_CALL(vkCreateRenderPass(vk.device, &renderPassInfo, nullptr, &renderPass));

        // ------------------------------ Create Framebuffers ------------------------------

        deferredGeoPipeline = resources.loadPipeline(renderPass, vk.swapChainExtent, "DeferredRenderGeo");
        deferredLightingPipeline = resources.loadPipeline(renderPass, vk.swapChainExtent, "DeferredRenderLighting");

        VulkDescriptorSetBuilder dsBuilder(vk);
        // this makes sue our .frag file matches what we do here
        dsBuilder.setDescriptorSetLayout(deferredLightingPipeline->descriptorSetLayout);
        dsBuilder.addFrameUBOs(scene->sceneUBOs.eyePos, VK_SHADER_STAGE_FRAGMENT_BIT, VulkShaderUBOBinding::EyePos);
        dsBuilder.addUniformBuffer(scene->sceneUBOs.pointLight, VK_SHADER_STAGE_FRAGMENT_BIT, VulkShaderUBOBinding::Lights);
        if (scene->pbrDebugUBO == nullptr)
            scene->pbrDebugUBO = std::make_shared<VulkUniformBuffer<VulkPBRDebugUBO>>(vk);
        dsBuilder.addUniformBuffer(*scene->pbrDebugUBO, VK_SHADER_STAGE_FRAGMENT_BIT, VulkShaderUBOBinding::PBRDebugUBO);
        if (scene->invViewProjUBO == nullptr)
            scene->invViewProjUBO = std::make_shared<VulkUniformBuffer<glm::mat4>>(vk);

        for (uint32_t frame = 0; frame < MAX_FRAMES_IN_FLIGHT; frame++) {
            auto geoBuf = geoBufs[frame];
            dsBuilder.addFrameImageSampler(
                frame, VK_SHADER_STAGE_FRAGMENT_BIT, VulkShaderTextureBinding::GBufAlbedo,
                geoBuf->gbufs.at(VulkGBufAttachment::Albedo)->view, textureSampler
            );
            dsBuilder.addFrameImageSampler(
                frame, VK_SHADER_STAGE_FRAGMENT_BIT, VulkShaderTextureBinding::GBufNormal,
                geoBuf->gbufs.at(VulkGBufAttachment::Normal)->view, textureSampler
            );
            dsBuilder.addFrameImageSampler(
                frame, VK_SHADER_STAGE_FRAGMENT_BIT, VulkShaderTextureBinding::GBufDepth,
                geoBuf->gbufs.at(VulkGBufAttachment::Depth)->view, textureSampler
            );
            dsBuilder.addFrameImageSampler(
                frame, VK_SHADER_STAGE_FRAGMENT_BIT, VulkShaderTextureBinding::GBufMaterial,
                geoBuf->gbufs.at(VulkGBufAttachment::Material)->view, textureSampler
            );
        }
        deferredLightingDescriptorSetInfo = dsBuilder.build();
    }

    void beginRenderToGBufs(VkCommandBuffer commandBuffer) {
        vk.beginDebugLabel(commandBuffer, "Deferred GBuffer Creation");
        std::array<VkClearValue, TEnumTraits<VulkGBufAttachment>::size + 1> clearValues{};
        clearValues[(int)VulkGBufAttachment::Depth].depthStencil = {1.0f, 0};

        VkRenderPassBeginInfo renderPassInfo{
            .sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
            .renderPass = renderPass,
            .framebuffer = frameBuffers[vk.currentFrame],
            .renderArea =
                {
                    .offset = {0, 0},
                    .extent = vk.swapChainExtent,
                },
            .clearValueCount = static_cast<uint32_t>(clearValues.size()),
            .pClearValues = clearValues.data()
        };

        vkCmdBeginRenderPass(commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
        VkViewport viewport{
            .x = 0.0f,
            .y = 0.0f,
            .width = (float)vk.swapChainExtent.width,
            .height = (float)vk.swapChainExtent.height,
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
        vkCmdBindDescriptorSets(
            commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, deferredLightingPipeline->pipelineLayout, 0, 1,
            &deferredLightingDescriptorSetInfo->descriptorSets[vk.currentFrame]->descriptorSet, 0, nullptr
        );
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