#pragma once

#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_vulkan.h"
#include <Vulk/Vulk.h>

static void imguiCheckResult(VkResult err) {
    if (err == 0)
        return;
    int e = (int)err;
    VULK_THROW_FMT("[vulkan] Error: VkResult = {}", e);
}

class VulkImGui {
    VkRenderPass renderPass;
    std::vector<VkImageView> swapChainImageViews;
    std::vector<VkFramebuffer> swapChainFramebuffers;
    VkDescriptorPool imguiDescriptorPool;
    Vulk& vk;
    GLFWwindow* window = nullptr;
    ImDrawData* drawData = nullptr;
    VkClearValue clearColor = {.color = {{0.45f, 0.55f, 0.60f, 1.00f}}};
    ImGuiIO* io = nullptr;

public:
    VulkImGui(Vulk& vk, GLFWwindow* window)
        : vk(vk)
        , window(window) {
        // renderPass
        {
            VkAttachmentDescription colorAttachment{};
            colorAttachment.format = vk.swapChainImageFormat;
            colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
            colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_LOAD;
            colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
            colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
            colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
            colorAttachment.initialLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR; // Start with the correct layout for swapchain images
            colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

            VkAttachmentReference colorAttachmentRef{};
            colorAttachmentRef.attachment = 0;
            colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

            VkSubpassDescription subpass{};
            subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
            subpass.colorAttachmentCount = 1;
            subpass.pColorAttachments = &colorAttachmentRef;

            VkSubpassDependency dependency{};
            dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
            dependency.dstSubpass = 0;
            dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
            dependency.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
            dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
            dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
            dependency.dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

            std::array<VkAttachmentDescription, 1> attachments = {colorAttachment};
            VkRenderPassCreateInfo renderPassInfo{};
            renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
            renderPassInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
            renderPassInfo.pAttachments = attachments.data();
            renderPassInfo.subpassCount = 1;
            renderPassInfo.pSubpasses = &subpass;
            renderPassInfo.dependencyCount = 1;
            renderPassInfo.pDependencies = &dependency;

            VK_CALL(vkCreateRenderPass(vk.device, &renderPassInfo, nullptr, &renderPass));
        }

        // image views
        swapChainImageViews.resize(vk.swapChainImages.size());
        for (uint32_t i = 0; i < vk.swapChainImages.size(); i++) {
            swapChainImageViews[i] = vk.createImageView(vk.swapChainImages[i], vk.swapChainImageFormat, VK_IMAGE_ASPECT_COLOR_BIT);
        }

        // frameBuffers
        swapChainFramebuffers.resize(swapChainImageViews.size());
        for (size_t i = 0; i < swapChainImageViews.size(); i++) {
            std::array<VkImageView, 1> fbAttachments = {swapChainImageViews[i]};

            VkFramebufferCreateInfo framebufferInfo{};
            framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
            framebufferInfo.renderPass = renderPass;
            framebufferInfo.attachmentCount = static_cast<uint32_t>(fbAttachments.size());
            framebufferInfo.pAttachments = fbAttachments.data();
            framebufferInfo.width = vk.swapChainExtent.width;
            framebufferInfo.height = vk.swapChainExtent.height;
            framebufferInfo.layers = 1;

            VK_CALL(vkCreateFramebuffer(vk.device, &framebufferInfo, nullptr, &swapChainFramebuffers[i]));
        }

        // Create Descriptor Pool
        // The example only requires a single combined image sampler descriptor for the font image and only uses one descriptor set (for that)
        // If you wish to load e.g. additional textures you may need to alter pools sizes.
        {
            VkDescriptorPoolSize pool_sizes[] = {
                {VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1},
            };
            VkDescriptorPoolCreateInfo pool_info = {};
            pool_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
            pool_info.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
            pool_info.maxSets = 1;
            pool_info.poolSizeCount = (uint32_t)IM_ARRAYSIZE(pool_sizes);
            pool_info.pPoolSizes = pool_sizes;
            VK_CALL(vkCreateDescriptorPool(vk.device, &pool_info, nullptr, &imguiDescriptorPool));
        }

        // must happen after making the window
        IMGUI_CHECKVERSION();
        ImGui::CreateContext();
        ImGui_ImplGlfw_InitForVulkan(window, true);
        ImGui_ImplVulkan_InitInfo init_info = {};
        init_info.Instance = vk.instance;
        init_info.PhysicalDevice = vk.physicalDevice;
        init_info.Device = vk.device;
        init_info.QueueFamily = vk.indices.graphicsFamily.value();
        init_info.Queue = vk.graphicsQueue;
        init_info.PipelineCache = nullptr;
        init_info.DescriptorPool = imguiDescriptorPool;
        // init_info.RenderPass = wd->RenderPass; hmmm
        init_info.Subpass = 0;
        init_info.MinImageCount = 2;
        init_info.ImageCount = (uint32_t)vk.swapChainImages.size();
        init_info.MSAASamples = VK_SAMPLE_COUNT_1_BIT;
        init_info.Allocator = nullptr;
        init_info.CheckVkResultFn = imguiCheckResult;
        init_info.RenderPass = renderPass;
        // ImGui_ImplVulkan_Init(&init_info, renderPass);
        ImGui_ImplVulkan_Init(&init_info);

        io = &ImGui::GetIO();
        io->ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard; // Enable Keyboard Controls
        io->ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;  // Enable Gamepad Controls

        // arbitrarily scale the font up
        io->FontGlobalScale = 2.f;
    }

public:
    // make your ImGui:: type calls between the begin and end
    void beginFrame() {
        VULK_ASSERT(!drawData, "drawData is not null, did you forget to call render?");
        // has to happen before any ImGUI calls
        ImGui_ImplVulkan_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();
    }

    void endFrame() {
        VULK_ASSERT(!drawData, "drawData is not null, did you forget to call render?");
        ImGui::Render();
        drawData = ImGui::GetDrawData();
    }

    void renderFrame(VkCommandBuffer commandBuffer, uint32_t imageIndex) {
        VULK_ASSERT(drawData, "drawData is null, did you forget to call endFrame?");
        // render the UI
        VkRenderPassBeginInfo info = {};
        info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
        info.renderPass = renderPass;
        info.framebuffer = swapChainFramebuffers[imageIndex];
        info.renderArea.extent = vk.swapChainExtent;

        vkCmdBeginRenderPass(commandBuffer, &info, VK_SUBPASS_CONTENTS_INLINE);
        ImGui_ImplVulkan_RenderDrawData(drawData, commandBuffer);
        vkCmdEndRenderPass(commandBuffer);
        drawData = nullptr;
    }

    ~VulkImGui() {
        ImGui_ImplVulkan_Shutdown();
        ImGui_ImplGlfw_Shutdown();
        ImGui::DestroyContext();
        vkDestroyDescriptorPool(vk.device, imguiDescriptorPool, nullptr);
        vkDestroyRenderPass(vk.device, renderPass, nullptr);
        for (auto framebuffer : swapChainFramebuffers) {
            vkDestroyFramebuffer(vk.device, framebuffer, nullptr);
        }
        for (auto imageView : swapChainImageViews) {
            vkDestroyImageView(vk.device, imageView, nullptr);
        }
    }
};
