#pragma once

#include <Vulk/Vulk.h>

#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_vulkan.h"
#include <stdio.h>  // printf, fprintf
#include <stdlib.h> // abort
#define GLFW_INCLUDE_NONE
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

static void glfw_error_callback(int error, const char *description) {
    fprintf(stderr, "GLFW Error %d: %s\n", error, description);
}
static void check_vk_result(VkResult err) {
    if (err == 0)
        return;
    fprintf(stderr, "[vulkan] Error: VkResult = %d\n", err);
    if (err < 0)
        abort();
}

class VulkRenderable {
  public:
    virtual void tick() = 0;
    virtual void drawFrame(VkCommandBuffer commandBuffer, VkFramebuffer frameBuffer) = 0;
};

class VulkImGuiRenderer {
  public:
    // make your ImGui:: type calls here
    virtual void drawUI() = 0;
};

class ExampleUI : public VulkImGuiRenderer {
    bool show_demo_window = true;
    bool show_another_window = false;
    float f = 0.0f;
    int counter = 0;
    ImGuiIO &io;

  public:
    ExampleUI(ImGuiIO &io) : io(io) {
    }

    void drawUI() {
        // 1. Show the big demo window (Most of the sample code is in ImGui::ShowDemoWindow()! You can browse its code to learn more about Dear ImGui!).
        if (show_demo_window)
            ImGui::ShowDemoWindow(&show_demo_window);

        // 2. Show a simple window that we create ourselves. We use a Begin/End pair to create a named window.
        {
            ImGui::Begin("Hello, world!"); // Create a window called "Hello, world!" and append into it.

            ImGui::Text("This is some useful text.");          // Display some text (you can use a format strings too)
            ImGui::Checkbox("Demo Window", &show_demo_window); // Edit bools storing our window open/close state
            ImGui::Checkbox("Another Window", &show_another_window);

            ImGui::SliderFloat("float", &f, 0.0f, 1.0f); // Edit 1 float using a slider from 0.0f to 1.0f
            // ImGui::ColorEdit3("clear color", (float *)&clear_color); // Edit 3 floats representing a color

            if (ImGui::Button("Button")) // Buttons return true when clicked (most widgets return true when edited/activated)
                counter++;
            ImGui::SameLine();
            ImGui::Text("counter = %d", counter);

            ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / io.Framerate, io.Framerate);
            ImGui::End();
        }

        // 3. Show another simple window.
        if (show_another_window) {
            ImGui::Begin(
                "Another Window",
                &show_another_window); // Pass a pointer to our bool variable (the window will have a closing button that will clear the bool when clicked)
            ImGui::Text("Hello from another window!");
            if (ImGui::Button("Close Me"))
                show_another_window = false;
            ImGui::End();
        }
    }
};

class VulkImGui : public Vulk {
    VkAllocationCallbacks *allocator = nullptr;
    uint32_t queueFamily = (uint32_t)-1;
    VkPipelineCache pipelineCache = VK_NULL_HANDLE;
    VkDescriptorPool descriptorPool = VK_NULL_HANDLE;

    ImGui_ImplVulkanH_Window mainWindowData;
    int minImageCount = 2;
    bool swapChainRebuild = false;

  public:
    ImVec4 clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.0f);
    std::shared_ptr<VulkRenderable> renderable;
    std::shared_ptr<VulkImGuiRenderer> uiRenderer;
    ImGuiIO *io;

    VulkImGui() {
        // Setup Dear ImGui contextIMGUI_CHECKVERSION();
        ImGui::CreateContext();
        io = &ImGui::GetIO();
        io->ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard; // Enable Keyboard Controls
        io->ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;  // Enable Gamepad Controls

        glfwSetErrorCallback(glfw_error_callback);
        initWindow();
        // initVulkan();

        ImVector<const char *> extensions;
        uint32_t extensions_count = 0;
        const char **glfw_extensions = glfwGetRequiredInstanceExtensions(&extensions_count);
        for (uint32_t i = 0; i < extensions_count; i++)
            extensions.push_back(glfw_extensions[i]);
        SetupVulkan(extensions);
        createCommandPool();
        createCommandBuffers();

        // Create Framebuffers
        int w, h;
        glfwGetFramebufferSize(window, &w, &h);
        ImGui_ImplVulkanH_Window *wd = &mainWindowData;
        SetupVulkanWindow(wd, surface, w, h);

        renderPass = wd->RenderPass;
        swapChain = wd->Swapchain;
        surface = wd->Surface;
        surfaceFormat = wd->SurfaceFormat;
        presentMode = wd->PresentMode;
        renderPass = wd->RenderPass;
        swapChainExtent = {static_cast<uint32_t>(w), static_cast<uint32_t>(h)};

        // Setup Dear ImGui style
        ImGui::StyleColorsDark();
        // ImGui::StyleColorsLight();

        // Setup Platform/Renderer backends
        ImGui_ImplGlfw_InitForVulkan(window, true);
        ImGui_ImplVulkan_InitInfo init_info = {};
        init_info.Instance = instance;
        init_info.PhysicalDevice = physicalDevice;
        init_info.Device = device;
        init_info.QueueFamily = queueFamily;
        init_info.Queue = graphicsQueue;
        init_info.PipelineCache = pipelineCache;
        init_info.DescriptorPool = descriptorPool;
        // init_info.RenderPass = wd->RenderPass; hmmm
        init_info.Subpass = 0;
        init_info.MinImageCount = minImageCount;
        init_info.ImageCount = wd->ImageCount;
        init_info.MSAASamples = VK_SAMPLE_COUNT_1_BIT;
        init_info.Allocator = allocator;
        init_info.CheckVkResultFn = check_vk_result;
        ImGui_ImplVulkan_Init(&init_info, wd->RenderPass);
    }

    void SetupVulkan(ImVector<const char *> instance_extensions) {
        VkResult err;

        createInstance();
        setupDebugMessenger();
        instance = instance;

        createSurface();
        pickPhysicalDevice();

        createLogicalDevice();
        queueFamily = indices.graphicsFamily.value();

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
            err = vkCreateDescriptorPool(device, &pool_info, allocator, &descriptorPool);
            check_vk_result(err);
        }
    }

    // All the ImGui_ImplVulkanH_XXX structures/functions are optional helpers used by the demo.
    // Your real engine/app may not use them.
    void SetupVulkanWindow(ImGui_ImplVulkanH_Window *wd, VkSurfaceKHR surface, int width, int height) {
        wd->Surface = surface;

        // Check for WSI support
        VkBool32 res;
        vkGetPhysicalDeviceSurfaceSupportKHR(physicalDevice, queueFamily, wd->Surface, &res);
        if (res != VK_TRUE) {
            fprintf(stderr, "Error no WSI support on physical device 0\n");
            exit(-1);
        }

        // Select Surface Format
        const VkFormat requestSurfaceImageFormat[] = {VK_FORMAT_B8G8R8A8_UNORM, VK_FORMAT_R8G8B8A8_UNORM, VK_FORMAT_B8G8R8_UNORM, VK_FORMAT_R8G8B8_UNORM};
        const VkColorSpaceKHR requestSurfaceColorSpace = VK_COLORSPACE_SRGB_NONLINEAR_KHR;
        wd->SurfaceFormat = ImGui_ImplVulkanH_SelectSurfaceFormat(physicalDevice, wd->Surface, requestSurfaceImageFormat,
                                                                  (size_t)IM_ARRAYSIZE(requestSurfaceImageFormat), requestSurfaceColorSpace);

        // Select Present Mode
        // #ifdef APP_USE_UNLIMITED_FRAME_RATE
        VkPresentModeKHR present_modes[] = {VK_PRESENT_MODE_MAILBOX_KHR, VK_PRESENT_MODE_IMMEDIATE_KHR, VK_PRESENT_MODE_FIFO_KHR};
        // #else
        //         VkPresentModeKHR present_modes[] = {VK_PRESENT_MODE_FIFO_KHR};
        // #endif
        wd->PresentMode = ImGui_ImplVulkanH_SelectPresentMode(physicalDevice, wd->Surface, &present_modes[0], IM_ARRAYSIZE(present_modes));
        // printf("[vulkan] Selected PresentMode = %d\n", wd->PresentMode);

        // Create SwapChain, RenderPass, Framebuffer, etc.
        IM_ASSERT(minImageCount >= 2);
        ImGui_ImplVulkanH_CreateOrResizeWindow(instance, physicalDevice, device, wd, queueFamily, allocator, width, height, minImageCount);
    }

    void CleanupVulkan() {
        vkDestroyDescriptorPool(device, descriptorPool, allocator);
    }

    void CleanupVulkanWindow() {
        ImGui_ImplVulkanH_DestroyWindow(instance, device, &mainWindowData, allocator);
    }

    void FrameRender(ImGui_ImplVulkanH_Window *wd, ImDrawData *draw_data) {
        VkResult err;

        VkSemaphore image_acquired_semaphore = wd->FrameSemaphores[wd->SemaphoreIndex].ImageAcquiredSemaphore;
        VkSemaphore render_complete_semaphore = wd->FrameSemaphores[wd->SemaphoreIndex].RenderCompleteSemaphore;
        err = vkAcquireNextImageKHR(device, wd->Swapchain, UINT64_MAX, image_acquired_semaphore, VK_NULL_HANDLE, &wd->FrameIndex);
        if (err == VK_ERROR_OUT_OF_DATE_KHR || err == VK_SUBOPTIMAL_KHR) {
            swapChainRebuild = true;
            return;
        }
        check_vk_result(err);

        ImGui_ImplVulkanH_Frame *fd = &wd->Frames[wd->FrameIndex];
        {
            err = vkWaitForFences(device, 1, &fd->Fence, VK_TRUE, UINT64_MAX); // wait indefinitely instead of periodically checking
            check_vk_result(err);

            err = vkResetFences(device, 1, &fd->Fence);
            check_vk_result(err);
        }
        {
            err = vkResetCommandPool(device, fd->CommandPool, 0);
            check_vk_result(err);
            VkCommandBufferBeginInfo info = {};
            info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
            info.flags |= VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
            err = vkBeginCommandBuffer(fd->CommandBuffer, &info);
            check_vk_result(err);
        }

        // the inner draw loop

        if (renderable) {
            renderable->drawFrame(fd->CommandBuffer, fd->Framebuffer);
        }

        // draw the ImGui UI
        {
            VkRenderPassBeginInfo info = {};
            info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
            info.renderPass = wd->RenderPass;
            info.framebuffer = fd->Framebuffer;
            info.renderArea.extent.width = wd->Width;
            info.renderArea.extent.height = wd->Height;
            info.clearValueCount = 1;
            info.pClearValues = &wd->ClearValue;
            vkCmdBeginRenderPass(fd->CommandBuffer, &info, VK_SUBPASS_CONTENTS_INLINE);
        }

        // Record dear imgui primitives into command buffer
        ImGui_ImplVulkan_RenderDrawData(draw_data, fd->CommandBuffer);

        // Submit command buffer
        vkCmdEndRenderPass(fd->CommandBuffer);

        err = vkEndCommandBuffer(fd->CommandBuffer);
        check_vk_result(err);

        {
            VkPipelineStageFlags wait_stage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
            VkSubmitInfo info = {};
            info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
            info.waitSemaphoreCount = 1;
            info.pWaitSemaphores = &image_acquired_semaphore;
            info.pWaitDstStageMask = &wait_stage;
            info.commandBufferCount = 1;
            info.pCommandBuffers = &fd->CommandBuffer;
            info.signalSemaphoreCount = 1;
            info.pSignalSemaphores = &render_complete_semaphore;

            err = vkQueueSubmit(graphicsQueue, 1, &info, fd->Fence);
            check_vk_result(err);
        }
    }

    void FramePresent(ImGui_ImplVulkanH_Window *wd) {
        if (swapChainRebuild)
            return;
        VkSemaphore render_complete_semaphore = wd->FrameSemaphores[wd->SemaphoreIndex].RenderCompleteSemaphore;
        VkPresentInfoKHR info = {};
        info.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
        info.waitSemaphoreCount = 1;
        info.pWaitSemaphores = &render_complete_semaphore;
        info.swapchainCount = 1;
        info.pSwapchains = &wd->Swapchain;
        info.pImageIndices = &wd->FrameIndex;
        VkResult err = vkQueuePresentKHR(graphicsQueue, &info);
        if (err == VK_ERROR_OUT_OF_DATE_KHR || err == VK_SUBOPTIMAL_KHR) {
            swapChainRebuild = true;
            return;
        }
        check_vk_result(err);
        wd->SemaphoreIndex = (wd->SemaphoreIndex + 1) % wd->SemaphoreCount; // Now we can use the next set of semaphores
    }

    // Main code
    void run() override {
        // Main loop
        while (!glfwWindowShouldClose(window)) {
            // Poll and handle events (inputs, window resize, etc.)
            // You can read the io.WantCaptureMouse, io.WantCaptureKeyboard flags to tell if dear imgui wants to use your inputs.
            // - When io.WantCaptureMouse is true, do not dispatch mouse input data to your main application, or clear/overwrite your copy of the mouse data.
            // - When io.WantCaptureKeyboard is true, do not dispatch keyboard input data to your main application, or clear/overwrite your copy of the keyboard
            // data. Generally you may always pass all inputs to dear imgui, and hide them from your application based on those two flags.
            glfwPollEvents();

            // Resize swap chain?
            if (swapChainRebuild) {
                int width, height;
                glfwGetFramebufferSize(window, &width, &height);
                if (width > 0 && height > 0) {
                    ImGui_ImplVulkan_SetMinImageCount(minImageCount);
                    ImGui_ImplVulkanH_CreateOrResizeWindow(instance, physicalDevice, device, &mainWindowData, queueFamily, allocator, width, height,
                                                           minImageCount);
                    mainWindowData.FrameIndex = 0;
                    swapChainRebuild = false;
                }
            }

            // Start the Dear ImGui frame
            ImGui_ImplVulkan_NewFrame();
            ImGui_ImplGlfw_NewFrame();
            ImGui::NewFrame();

            if (uiRenderer)
                uiRenderer->drawUI();

            if (renderable)
                renderable->tick();

            // Rendering
            ImGui::Render();
            ImDrawData *draw_data = ImGui::GetDrawData();
            const bool is_minimized = (draw_data->DisplaySize.x <= 0.0f || draw_data->DisplaySize.y <= 0.0f);
            if (!is_minimized) {
                ImGui_ImplVulkanH_Window *wd = &mainWindowData;
                wd->ClearValue.color.float32[0] = clear_color.x * clear_color.w;
                wd->ClearValue.color.float32[1] = clear_color.y * clear_color.w;
                wd->ClearValue.color.float32[2] = clear_color.z * clear_color.w;
                wd->ClearValue.color.float32[3] = clear_color.w;
                currentFrame = wd->FrameIndex;
                FrameRender(wd, draw_data);
                FramePresent(wd);
            }
        }

        // Cleanup
        VkResult err = vkDeviceWaitIdle(device);
        check_vk_result(err);
        ImGui_ImplVulkan_Shutdown();
        ImGui_ImplGlfw_Shutdown();
        ImGui::DestroyContext();

        CleanupVulkanWindow();
        CleanupVulkan();

        glfwDestroyWindow(window);
        glfwTerminate();
    }

    virtual void init() {
    }
    virtual void drawFrame(VkCommandBuffer commandBuffer, VkFramebuffer frameBuffer) {
    }
    virtual void cleanup() {
    }
};