#include "Vulk/VulkDeferredRenderpass.h"

namespace vulk {

VulkDeferredRenderpass::VulkDeferredRenderpass(Vulk& vkIn, VulkResources& resources, VulkScene& scene) : vk(vkIn) {
    // TODO: remove
    logger->set_level(spdlog::level::debug);
    geoBufs = std::make_unique<VulkGBufs>(vk);

    // Attachments: We need the gbufs and the swapchain image as attachments to the renderpass
    // --------------------------------------------------------------------------
    std::vector<VkAttachmentDescription> attachments(TEnumTraits<GBufAtmtIdx>::size);

    for (const GBufAtmtIdx atmt : geoBufs->gbufs | std::views::keys) {
        VULK_ASSERT(atmt != GBufAtmtIdx::Color);
        bool isDepth                  = atmt == GBufAtmtIdx::Depth;
        size_t i                      = (size_t)atmt;
        attachments[i].format         = geoBufs->gbufs.at(atmt)->format;
        attachments[i].samples        = VK_SAMPLE_COUNT_1_BIT;
        attachments[i].loadOp         = VK_ATTACHMENT_LOAD_OP_CLEAR;
        attachments[i].storeOp        = VK_ATTACHMENT_STORE_OP_DONT_CARE;  // VK_ATTACHMENT_STORE_OP_STORE;
        attachments[i].stencilLoadOp  = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        attachments[i].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        attachments[i].initialLayout  = VK_IMAGE_LAYOUT_UNDEFINED;
        // these aren't actually read from the shaders as textures, so we don't need to make them shader read only
        attachments[i].finalLayout =
            isDepth ? VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL : VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
        // isDepth ? VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL : VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    }

    // we also need the swapchain image
    VkAttachmentDescription colorAttachment{
        .format         = vk.swapChainImageFormat,
        .samples        = VK_SAMPLE_COUNT_1_BIT,
        .loadOp         = VK_ATTACHMENT_LOAD_OP_CLEAR,
        .storeOp        = VK_ATTACHMENT_STORE_OP_STORE,
        .stencilLoadOp  = VK_ATTACHMENT_LOAD_OP_DONT_CARE,  // VK_ATTACHMENT_LOAD_OP_CLEAR, why not this?
        .stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
        .initialLayout  = VK_IMAGE_LAYOUT_UNDEFINED,
        .finalLayout    = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
    };
    attachments[(int)GBufAtmtIdx::Color] = colorAttachment;

    // ============================== Subpass Descriptions ==============================

    // used in both subpasses
    VkAttachmentReference depthRef = {
        .attachment = (uint32_t)GBufAtmtIdx::Depth,
        .layout     = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
    };

    // ------------------------------ First Subpass: Geometry Pass ------------------------------

    std::vector<VkAttachmentReference> geoAttachmentRefs;
    for (uint32_t i = 0; i < attachments.size(); i++) {
        if (i == (uint32_t)GBufAtmtIdx::Depth) {
            continue;  // passed separately
        }

        geoAttachmentRefs.push_back({
            .attachment = i,
            .layout     = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
        });
    }

    VkSubpassDescription geoSubpassDescription = {
        .pipelineBindPoint       = VK_PIPELINE_BIND_POINT_GRAPHICS,
        .colorAttachmentCount    = (uint32_t)geoAttachmentRefs.size(),
        .pColorAttachments       = geoAttachmentRefs.data(),
        .pDepthStencilAttachment = &depthRef,
    };

    // ------------------------------ Second Subpass: Lighting Pass ------------------------------

    // we still need this as an attachment reference
    VkAttachmentReference colorAttachmentRef{
        .attachment = (uint32_t)GBufAtmtIdx::Color,
        .layout     = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
    };

    // but the gbufs are now input attachments
    std::vector<VkAttachmentReference> lightingAttachments{{
        {
            .attachment = (uint32_t)GBufAtmtIdx::Albedo,
            .layout     = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
        },
        {
            .attachment = (uint32_t)GBufAtmtIdx::Normal,
            .layout     = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
        },
        {
            .attachment = (uint32_t)GBufAtmtIdx::Material,
            .layout     = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
        },
    }};

    VkSubpassDescription lightingSubpassDescription{
        .pipelineBindPoint       = VK_PIPELINE_BIND_POINT_GRAPHICS,
        .inputAttachmentCount    = (uint32_t)lightingAttachments.size(),
        .pInputAttachments       = lightingAttachments.data(),
        .colorAttachmentCount    = 1,
        .pColorAttachments       = &colorAttachmentRef,
        .pDepthStencilAttachment = &depthRef,
    };

    // ------------------------------ Subpass Dependencies ------------------------------

    std::array<VkSubpassDependency, 4> dependencies;
    // This makes sure that writes to the depth image are done before we try to write to it again
    dependencies[0].srcSubpass      = VK_SUBPASS_EXTERNAL;
    dependencies[0].dstSubpass      = 0;
    dependencies[0].srcStageMask    = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
    dependencies[0].dstStageMask    = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
    dependencies[0].srcAccessMask   = 0;
    dependencies[0].dstAccessMask   = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
    dependencies[0].dependencyFlags = 0;

    dependencies[1].srcSubpass      = VK_SUBPASS_EXTERNAL;
    dependencies[1].dstSubpass      = 0;
    dependencies[1].srcStageMask    = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependencies[1].dstStageMask    = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependencies[1].srcAccessMask   = 0;
    dependencies[1].dstAccessMask   = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
    dependencies[1].dependencyFlags = 0;

    // This dependency transitions the input attachment from color attachment to input attachment read
    dependencies[2].srcSubpass      = 0;
    dependencies[2].dstSubpass      = 1;
    dependencies[2].srcStageMask    = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependencies[2].dstStageMask    = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
    dependencies[2].srcAccessMask   = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
    dependencies[2].dstAccessMask   = VK_ACCESS_INPUT_ATTACHMENT_READ_BIT;
    dependencies[2].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

    // for when we add a transparent subpass
    // dependencies[3].srcSubpass = 1;
    // dependencies[3].dstSubpass = 2;
    // dependencies[3].srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    // dependencies[3].dstStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
    // dependencies[3].srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
    // dependencies[3].dstAccessMask = VK_ACCESS_INPUT_ATTACHMENT_READ_BIT;
    // dependencies[3].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

    dependencies[3].srcSubpass      = 1;
    dependencies[3].dstSubpass      = VK_SUBPASS_EXTERNAL;
    dependencies[3].srcStageMask    = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependencies[3].dstStageMask    = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
    dependencies[3].srcAccessMask   = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
    dependencies[3].dstAccessMask   = VK_ACCESS_MEMORY_READ_BIT;
    dependencies[3].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

    // ------------------------------ Create Render Pass ------------------------------
    std::array<VkSubpassDescription, 2> subpassDescriptions{geoSubpassDescription, lightingSubpassDescription};

    VkRenderPassCreateInfo renderPassInfo = {
        .sType           = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,
        .attachmentCount = static_cast<uint32_t>(attachments.size()),
        .pAttachments    = attachments.data(),
        .subpassCount    = static_cast<uint32_t>(subpassDescriptions.size()),
        .pSubpasses      = subpassDescriptions.data(),
        .dependencyCount = static_cast<uint32_t>(dependencies.size()),
        .pDependencies   = dependencies.data(),
    };
    VK_CALL(vkCreateRenderPass(vk.device, &renderPassInfo, nullptr, &renderPass));

    // ------------------------------ Load Pipelines ------------------------------

    deferredGeoPipeline      = resources.loadPipeline(renderPass, vk.swapChainExtent, "DeferredRenderGeo");
    deferredLightingPipeline = resources.loadPipeline(renderPass, vk.swapChainExtent, "DeferredRenderLighting");

    VulkPipeline const& pipeline      = *deferredLightingPipeline;
    deferredLightingDescriptorSetInfo = resources.createDSInfoFromPipeline(pipeline, &scene, nullptr, nullptr, this);

    // ------------------------------ Create Framebuffers ------------------------------

    VkImageView atmts[5];

    VkFramebufferCreateInfo frameBufferCreateInfo = {};
    frameBufferCreateInfo.sType                   = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
    frameBufferCreateInfo.renderPass              = renderPass;
    frameBufferCreateInfo.attachmentCount         = 5;
    frameBufferCreateInfo.pAttachments            = atmts;
    frameBufferCreateInfo.width                   = vk.swapChainExtent.width;
    frameBufferCreateInfo.height                  = vk.swapChainExtent.height;
    frameBufferCreateInfo.layers                  = 1;

    // Create frame buffers for every swap chain image
    frameBuffers.resize(vk.swapChainImageViews.size());
    for (uint32_t i = 0; i < vk.swapChainImageViews.size(); i++) {
        atmts[(int)GBufAtmtIdx::Color]    = vk.swapChainImageViews[i];
        atmts[(int)GBufAtmtIdx::Albedo]   = geoBufs->gbufViews[(int)GBufAtmtIdx::Albedo];
        atmts[(int)GBufAtmtIdx::Normal]   = geoBufs->gbufViews[(int)GBufAtmtIdx::Normal];
        atmts[(int)GBufAtmtIdx::Material] = geoBufs->gbufViews[(int)GBufAtmtIdx::Material];
        atmts[(int)GBufAtmtIdx::Depth]    = geoBufs->gbufViews[(int)GBufAtmtIdx::Depth];
        VK_CALL(vkCreateFramebuffer(vk.device, &frameBufferCreateInfo, nullptr, &frameBuffers[i]));
    }
}

}  // namespace vulk