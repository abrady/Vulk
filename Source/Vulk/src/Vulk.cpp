#include "Vulk/Vulk.h"

#include <GLFW/glfw3.h>

#include <iostream>
#include "VulkImGui.h"

DECLARE_FILE_LOGGER();

PFN_vkCmdBeginDebugUtilsLabelEXT pfnVkCmdBeginDebugUtilsLabelEXT{nullptr};
PFN_vkCmdEndDebugUtilsLabelEXT pfnVkCmdEndDebugUtilsLabelEXT{nullptr};
PFN_vkCmdInsertDebugUtilsLabelEXT pfnVkCmdInsertDebugUtilsLabelEXT{nullptr};

class DebugMouseEventHandler : public MouseEventHandler {
   public:
    void onClick(double xpos, double ypos, MouseEventContext const& ctxt) override {
        std::cout << "Click at: (" << xpos << ", " << ypos << ")"
                  << " Shift: " << ctxt.shift << " Control: " << ctxt.control << " Alt: " << ctxt.alt << std::endl;
    }

    void onDoubleClick(double xpos, double ypos, MouseEventContext const& ctxt) override {
        std::cout << "Double Click at: (" << xpos << ", " << ypos << ")"
                  << " Shift: " << ctxt.shift << " Control: " << ctxt.control << " Alt: " << ctxt.alt << std::endl;
    }

    void onMouseDown(double xpos, double ypos, MouseEventContext const& ctxt) override {
        std::cout << "Mouse Down at: (" << xpos << ", " << ypos << ")"
                  << " Shift: " << ctxt.shift << " Control: " << ctxt.control << " Alt: " << ctxt.alt << std::endl;
    }

    void onMouseMove(double xpos, double ypos, MouseEventContext const& ctxt) override {
        std::cout << "Mouse Move to: (" << xpos << ", " << ypos << ")"
                  << " Shift: " << ctxt.shift << " Control: " << ctxt.control << " Alt: " << ctxt.alt << std::endl;
    }

    void onMouseUp(double xpos, double ypos, MouseEventContext const& ctxt) override {
        std::cout << "Mouse Up at: (" << xpos << ", " << ypos << ")"
                  << " Shift: " << ctxt.shift << " Control: " << ctxt.control << " Alt: " << ctxt.alt << std::endl;
    }

    void onDrag(double xpos, double ypos, MouseDragContext const& drag, MouseEventContext const& ctxt) override {
        std::cout << "Dragging at: (" << xpos << ", " << ypos << ")"
                  << " Shift: " << ctxt.shift << "is dragging: " << ctxt.isDragging << " Control: " << ctxt.control
                  << " Alt: " << ctxt.alt << " Drag start: (" << drag.dragStartX << ", " << drag.dragStartY << ")" << std::endl;
    }

    void onDragStart(double xpos, double ypos, MouseEventContext const& ctxt) override {
        std::cout << "Drag Start at: (" << xpos << ", " << ypos << ")"
                  << " Shift: " << ctxt.shift << " Control: " << ctxt.control << " Alt: " << ctxt.alt << std::endl;
    }

    void onDragEnd(double xpos, double ypos, MouseEventContext const& ctxt) override {
        std::cout << "Drag End at: (" << xpos << ", " << ypos << ")"
                  << " Shift: " << ctxt.shift << " Control: " << ctxt.control << " Alt: " << ctxt.alt << std::endl;
    }
};

static MouseEventHandler* eventHandler = nullptr;
static std::shared_ptr<MouseEventContext> mouseEventctxt;
static std::shared_ptr<MouseDragContext> dragContext;

void setMouseEventHandler(MouseEventHandler* handler) {
    if (!handler) {
        clearMouseEventHandler();
        return;
    }
    eventHandler   = handler;
    mouseEventctxt = std::make_shared<MouseEventContext>();
    dragContext    = std::make_shared<MouseDragContext>();
}

void clearMouseEventHandler() {
    eventHandler = nullptr;
    mouseEventctxt.reset();
    dragContext.reset();
}

void mouse_button_callback(GLFWwindow* window, int button, int action, int mods) {
    if (!eventHandler) {
        return;
    }
    MouseEventContext& ctxt = *mouseEventctxt;
    double xpos, ypos;
    glfwGetCursorPos(window, &xpos, &ypos);
    ctxt.shift   = mods & GLFW_MOD_SHIFT;
    ctxt.control = mods & GLFW_MOD_CONTROL;
    ctxt.alt     = mods & GLFW_MOD_ALT;

    if (button == GLFW_MOUSE_BUTTON_LEFT) {
        if (action == GLFW_PRESS) {
            eventHandler->onMouseDown(xpos, ypos, ctxt);

            double currentTime = glfwGetTime();
            if (currentTime - ctxt.lastClickTime < 0.3) {  // Assuming 0.3 seconds as double-click interval
                eventHandler->onDoubleClick(xpos, ypos, ctxt);
            }
            ctxt.lastClickTime = currentTime;

            // Start dragging
            dragContext             = std::make_shared<MouseDragContext>();
            ctxt.isDragging         = true;
            dragContext->dragStartX = xpos;
            dragContext->dragStartY = ypos;
            eventHandler->onDragStart(xpos, ypos, ctxt);
        } else if (action == GLFW_RELEASE) {
            eventHandler->onMouseUp(xpos, ypos, ctxt);

            if (ctxt.isDragging) {
                ctxt.isDragging = false;
                eventHandler->onDragEnd(xpos, ypos, ctxt);
                dragContext.reset();
            } else {
                eventHandler->onClick(xpos, ypos, ctxt);
            }
        }
    }
}

void cursor_position_callback(GLFWwindow* window, double xpos, double ypos) {
    if (!eventHandler) {
        return;
    }

    // Get the window size and framebuffer size
    int windowWidth, windowHeight;
    glfwGetWindowSize(window, &windowWidth, &windowHeight);

    // Calculate the scaling factor
    double xscale = 1.0 / (double)windowWidth;
    double yscale = 1.0 / (double)windowHeight;

    MouseEventContext& ctxt = *mouseEventctxt;
    double currentTime      = glfwGetTime();
    if (dragContext) {  // calculate dxdt and dydt (velocity of mouse movement
        double dt   = currentTime - ctxt.lastCursorPosTime;
        double dxdt = (xpos - ctxt.lastX) / dt * xscale;
        double dydt = (ypos - ctxt.lastY) / dt * yscale;
        // blend dxdt and dydt with previous values
        dragContext->dxdt = 0.8 * dragContext->dxdt + 0.2 * dxdt;
        dragContext->dydt = 0.8 * dragContext->dydt + 0.2 * dydt;
    }

    ctxt.shift = glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS || glfwGetKey(window, GLFW_KEY_RIGHT_SHIFT) == GLFW_PRESS;
    ctxt.control =
        glfwGetKey(window, GLFW_KEY_LEFT_CONTROL) == GLFW_PRESS || glfwGetKey(window, GLFW_KEY_RIGHT_CONTROL) == GLFW_PRESS;
    ctxt.alt = glfwGetKey(window, GLFW_KEY_LEFT_ALT) == GLFW_PRESS || glfwGetKey(window, GLFW_KEY_RIGHT_ALT) == GLFW_PRESS;

    eventHandler->onMouseMove(xpos, ypos, ctxt);
    if (ctxt.isDragging) {
        eventHandler->onDrag(xpos, ypos, *dragContext, ctxt);
    }

    ctxt.lastCursorPosTime = currentTime;
    ctxt.lastX             = xpos;
    ctxt.lastY             = ypos;
}

static const std::vector<const char*> validationLayers = {"VK_LAYER_KHRONOS_validation"};

static const std::vector<const char*> deviceExtensions = {
    VK_KHR_SWAPCHAIN_EXTENSION_NAME,
};

Vulk::Vulk() {
    initWindow();
    initVulkan();
}

void Vulk::run() {
    lastFrameTime = std::chrono::steady_clock::now();
    while (!glfwWindowShouldClose(window)) {
        handleEvents();
        glfwPollEvents();

        if (uiRenderer) {
            uiRenderer->beginFrame();
        }

        if (renderable) {
            renderable->tick();
        }

        if (uiRenderer) {
            uiRenderer->endFrame();
        }

        render();

        auto now          = std::chrono::steady_clock::now();
        auto elapsed_time = std::chrono::duration_cast<std::chrono::milliseconds>(now - lastFrameTime);
        if (elapsed_time < msPerFrame) {  // 16ms = 60fps
            std::this_thread::sleep_for(msPerFrame - elapsed_time);
        }
        lastFrameTime = std::chrono::steady_clock::now();
    }

    vkDeviceWaitIdle(device);
    cleanupVulkan();  // calls cleanup
}

void Vulk::createBuffer(VkDeviceSize size,
                        VkBufferUsageFlags usage,
                        VkMemoryPropertyFlags properties,
                        VkBuffer& buffer,
                        VkDeviceMemory& bufferMemory) {
    VkBufferCreateInfo bufferInfo{};
    bufferInfo.sType       = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferInfo.size        = size;
    bufferInfo.usage       = usage;
    bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    VK_CALL(vkCreateBuffer(device, &bufferInfo, nullptr, &buffer));

    VkMemoryRequirements memRequirements;
    vkGetBufferMemoryRequirements(device, buffer, &memRequirements);

    VkMemoryAllocateInfo allocInfo{};
    allocInfo.sType           = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize  = memRequirements.size;
    allocInfo.memoryTypeIndex = findMemoryType(memRequirements.memoryTypeBits, properties);

    VK_CALL(vkAllocateMemory(device, &allocInfo, nullptr, &bufferMemory));

    vkBindBufferMemory(device, buffer, bufferMemory, 0);
}

void Vulk::copyMemToBuffer(void const* srcBuffer, VkBuffer dstBuffer, VkDeviceSize size) {
    VkBuffer stagingBuffer;
    VkDeviceMemory stagingBufferMemory;
    createBuffer(size,
                 VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                 VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                 stagingBuffer,
                 stagingBufferMemory);
    void* data;
    vkMapMemory(device, stagingBufferMemory, 0, size, 0, &data);
    memcpy(data, srcBuffer, size);
    vkUnmapMemory(device, stagingBufferMemory);
    copyBuffer(stagingBuffer, dstBuffer, size);
    vkDestroyBuffer(device, stagingBuffer, nullptr);
    vkFreeMemory(device, stagingBufferMemory, nullptr);
}

void Vulk::copyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size) {
    VkCommandBuffer commandBuffer = beginSingleTimeCommands();

    VkBufferCopy copyRegion{};
    copyRegion.size = size;
    vkCmdCopyBuffer(commandBuffer, srcBuffer, dstBuffer, 1, &copyRegion);

    endSingleTimeCommands(commandBuffer);
}

void Vulk::copyImageToBuffer(VkImage image, VkBuffer buffer, uint32_t width, uint32_t height) {
    VkCommandBuffer commandBuffer = beginSingleTimeCommands();

    VkBufferImageCopy region{};
    region.bufferOffset      = 0;
    region.bufferRowLength   = 0;
    region.bufferImageHeight = 0;

    region.imageSubresource.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
    region.imageSubresource.mipLevel       = 0;
    region.imageSubresource.baseArrayLayer = 0;
    region.imageSubresource.layerCount     = 1;

    region.imageOffset = {0, 0, 0};
    region.imageExtent = {width, height, 1};

    vkCmdCopyImageToBuffer(commandBuffer, image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, buffer, 1, &region);

    endSingleTimeCommands(commandBuffer);
}

void Vulk::copyBufferToMem(VkBuffer srcBuffer, void* dstBuffer, VkDeviceSize size) {
    // Create a staging buffer with properties suitable for CPU access
    // Copy the data from the source buffer to the staging buffer
    // Map the staging buffer memory and copy the data to the destination buffer in CPU memory
    VkBuffer stagingBuffer;
    VkDeviceMemory stagingBufferMemory;
    createBuffer(size,
                 VK_BUFFER_USAGE_TRANSFER_DST_BIT,
                 VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                 stagingBuffer,
                 stagingBufferMemory);
    copyBuffer(srcBuffer, stagingBuffer, size);
    void* data;
    vkMapMemory(device, stagingBufferMemory, 0, size, 0, &data);
    memcpy(dstBuffer, data, (size_t)size);
    vkUnmapMemory(device, stagingBufferMemory);
    // Clean up the staging buffer and its memory
    vkDestroyBuffer(device, stagingBuffer, nullptr);
    vkFreeMemory(device, stagingBufferMemory, nullptr);
}

void Vulk::copyImageToMem(VkImage image, void* dstBuffer, uint32_t width, uint32_t height, VkDeviceSize dstEltSize) {
    VkBuffer stagingBuffer;
    VkDeviceMemory stagingBufferMemory;
    VkDeviceSize size = width * height * dstEltSize;
    createBuffer(size,
                 VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
                 VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                 stagingBuffer,
                 stagingBufferMemory);

    VkCommandBuffer commandBuffer = beginSingleTimeCommands();

    // VkImageMemoryBarrier barrier{};
    // barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    // barrier.oldLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL; // Previous layout used in the render pass
    // barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;     // New layout for copying operation
    // barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    // barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    // barrier.image = image;
    // barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    // barrier.subresourceRange.baseMipLevel = 0;
    // barrier.subresourceRange.levelCount = 1;
    // barrier.subresourceRange.baseArrayLayer = 0;
    // barrier.subresourceRange.layerCount = 1;

    // barrier.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT; // Operations to wait on (color attachment writes)
    // barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;          // Operations that should wait on this barrier
    // (transfer reads)

    // VkPipelineStageFlags sourceStage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT; // Stage of pipeline operations
    // that involve color attachment writes VkPipelineStageFlags destinationStage = VK_PIPELINE_STAGE_TRANSFER_BIT; //
    // Stage of pipeline operations that involve transfer reads vkCmdPipelineBarrier(commandBuffer, sourceStage,
    // destinationStage, 0, 0, nullptr, 0, nullptr, 1, &barrier);

    VkBufferImageCopy region{};
    region.bufferOffset      = 0;
    region.bufferRowLength   = 0;
    region.bufferImageHeight = 0;

    region.imageSubresource.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
    region.imageSubresource.mipLevel       = 0;
    region.imageSubresource.baseArrayLayer = 0;
    region.imageSubresource.layerCount     = 1;

    region.imageOffset = {0, 0, 0};
    region.imageExtent = {width, height, 1};

    vkCmdCopyImageToBuffer(commandBuffer, image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, stagingBuffer, 1, &region);

    // transitionImageLayout(commandBuffer, image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
    // VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);

    endSingleTimeCommands(commandBuffer);

    copyBufferToMem(stagingBuffer, dstBuffer, size);

    vkDestroyBuffer(device, stagingBuffer, nullptr);
    vkFreeMemory(device, stagingBufferMemory, nullptr);
}

void Vulk::initWindow() {
    glfwInit();
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

    window = glfwCreateWindow(WINDOW_WIDTH, WINDOW_HEIGHT, "Vulkan", nullptr, nullptr);
    glfwSetWindowUserPointer(window, this);
    glfwSetFramebufferSizeCallback(window, framebufferResizeCallback);
    glfwSetKeyCallback(window, dispatchKeyCallback);
    glfwSetMouseButtonCallback(window, mouse_button_callback);
    glfwSetCursorPosCallback(window, cursor_position_callback);
}

void Vulk::framebufferResizeCallback(GLFWwindow* window, int /*width*/, int /*height*/) {
    auto app                = reinterpret_cast<Vulk*>(glfwGetWindowUserPointer(window));
    app->framebufferResized = true;
}

void Vulk::initVulkan() {
    createInstance();
    setupDebug();
    createSurface();
    pickPhysicalDevice();
    createLogicalDevice();
    createSwapChain();
    createImageViews();
    createRenderPass();
    createCommandPool();
    createCommandBuffers();
    createDepthResources();
    createFramebuffers();
    createSyncObjects();

    uiRenderer = std::make_shared<VulkImGui>(*this, window);
}

void Vulk::cleanupSwapChain() {
    vkDestroyImageView(device, depthImageView, nullptr);
    vkDestroyImage(device, depthImage, nullptr);
    vkFreeMemory(device, depthImageMemory, nullptr);

    for (auto framebuffer : swapChainFramebuffers) {
        vkDestroyFramebuffer(device, framebuffer, nullptr);
    }

    for (auto imageView : swapChainImageViews) {
        vkDestroyImageView(device, imageView, nullptr);
    }

    vkDestroySwapchainKHR(device, swapChain, nullptr);
}

void Vulk::cleanupVulkan() {
    VK_CALL(vkDeviceWaitIdle(device));

    renderable.reset();
    uiRenderer.reset();

    cleanupSwapChain();

    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
        vkDestroySemaphore(device, renderFinishedSemaphores[i], nullptr);
        vkDestroySemaphore(device, imageAvailableSemaphores[i], nullptr);
        vkDestroyFence(device, inFlightFences[i], nullptr);
    }

    vkDestroyCommandPool(device, commandPool, nullptr);
    vkDestroyRenderPass(device, renderPass, nullptr);
    vkDestroyDevice(device, nullptr);

    if (enableValidationLayers) {
        DestroyDebugUtilsMessengerEXT(instance, debugMessenger, nullptr);
    }

    vkDestroySurfaceKHR(instance, surface, nullptr);
    vkDestroyInstance(instance, nullptr);

    glfwDestroyWindow(window);

    glfwTerminate();
}

static const std::unordered_map<VkFormat, uint32_t> numChannelsFromFormat = {
    {VK_FORMAT_R8_UNORM, 1},
    {VK_FORMAT_R8G8_UNORM, 2},
    {VK_FORMAT_R8G8B8_UNORM, 3},
    {VK_FORMAT_R8G8B8_SRGB, 3},
    {VK_FORMAT_R8G8B8A8_UNORM, 4},
    {VK_FORMAT_R8G8B8A8_SRGB, 4},
};

VkImage Vulk::createTextureImage(char const* texture_path,
                                 VkDeviceMemory& textureImageMemory,
                                 VkImage& textureImage,
                                 bool isUNORM,
                                 VkFormat& formatOut) {
    int texWidth, texHeight, texChannels;
    stbi_uc* pixels        = stbi_load(texture_path, &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);
    VkDeviceSize imageSize = texWidth * texHeight * 4;  // not texChannels because we always load 4 channels because
                                                        // drivers prefer 32 bit aligned data...
    assert(pixels);
    VkFormat format;
    if (isUNORM) {
        format = VK_FORMAT_R8G8B8A8_UNORM;
    } else {
        format = VK_FORMAT_R8G8B8A8_SRGB;
    }
    formatOut = format;

    VkBuffer stagingBuffer;
    VkDeviceMemory stagingBufferMemory;
    createBuffer(imageSize,
                 VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                 VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                 stagingBuffer,
                 stagingBufferMemory);

    void* data;
    vkMapMemory(device, stagingBufferMemory, 0, imageSize, 0, &data);
    memcpy(data, pixels, static_cast<size_t>(imageSize));
    vkUnmapMemory(device, stagingBufferMemory);

    stbi_image_free(pixels);

    createImage(texWidth,
                texHeight,
                format,
                VK_IMAGE_TILING_OPTIMAL,
                VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
                VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                textureImage,
                textureImageMemory);

    VkCommandBuffer commandBuffer = beginSingleTimeCommands();
    transitionImageLayout(commandBuffer, textureImage, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
    endSingleTimeCommands(commandBuffer);

    copyBufferToImage(stagingBuffer, textureImage, static_cast<uint32_t>(texWidth), static_cast<uint32_t>(texHeight));

    commandBuffer = beginSingleTimeCommands();
    transitionImageLayout(commandBuffer,
                          textureImage,
                          VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                          VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
    endSingleTimeCommands(commandBuffer);

    vkDestroyBuffer(device, stagingBuffer, nullptr);
    vkFreeMemory(device, stagingBufferMemory, nullptr);
    return textureImage;
}

uint32_t Vulk::findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties) {
    VkPhysicalDeviceMemoryProperties memProperties;
    vkGetPhysicalDeviceMemoryProperties(physicalDevice, &memProperties);

    for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++) {
        if ((typeFilter & (1 << i)) && (memProperties.memoryTypes[i].propertyFlags & properties) == properties) {
            return i;
        }
    }

    VULK_THROW("failed to find suitable memory type!");
}

VkShaderModule Vulk::createShaderModule(const std::vector<char>& code) {
    VkShaderModuleCreateInfo createInfo{};
    createInfo.sType    = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    createInfo.codeSize = code.size();
    createInfo.pCode    = reinterpret_cast<const uint32_t*>(code.data());

    VkShaderModule shaderModule;
    VK_CALL(vkCreateShaderModule(device, &createInfo, nullptr, &shaderModule));

    return shaderModule;
}

VkDescriptorSet Vulk::createDescriptorSet(VkDescriptorSetLayout descriptorSetLayout, VkDescriptorPool descriptorPool) {
    VkDescriptorSetLayout layouts[] = {descriptorSetLayout};
    VkDescriptorSetAllocateInfo allocInfo{};
    allocInfo.sType              = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocInfo.descriptorPool     = descriptorPool;
    allocInfo.descriptorSetCount = 1;
    allocInfo.pSetLayouts        = layouts;

    VkDescriptorSet descriptorSet;
    VK_CALL(vkAllocateDescriptorSets(device, &allocInfo, &descriptorSet));
    return descriptorSet;
}

VkSampler Vulk::createTextureSampler() {
    VkPhysicalDeviceProperties properties{};
    vkGetPhysicalDeviceProperties(physicalDevice, &properties);

    VkSamplerCreateInfo samplerInfo{};
    samplerInfo.sType                   = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    samplerInfo.magFilter               = VK_FILTER_LINEAR;
    samplerInfo.minFilter               = VK_FILTER_LINEAR;
    samplerInfo.addressModeU            = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    samplerInfo.addressModeV            = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    samplerInfo.addressModeW            = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    samplerInfo.anisotropyEnable        = VK_TRUE;
    samplerInfo.maxAnisotropy           = properties.limits.maxSamplerAnisotropy;
    samplerInfo.borderColor             = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
    samplerInfo.unnormalizedCoordinates = VK_FALSE;
    samplerInfo.compareEnable           = VK_FALSE;
    samplerInfo.compareOp               = VK_COMPARE_OP_ALWAYS;
    samplerInfo.mipmapMode              = VK_SAMPLER_MIPMAP_MODE_LINEAR;

    VkSampler textureSampler;
    VK_CALL(vkCreateSampler(device, &samplerInfo, nullptr, &textureSampler));
    return textureSampler;
}

VkImageView Vulk::createImageView(VkImage image, VkFormat format, VkImageAspectFlags aspectFlags) {
    VkImageViewCreateInfo viewInfo{};
    viewInfo.sType                           = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    viewInfo.image                           = image;
    viewInfo.viewType                        = VK_IMAGE_VIEW_TYPE_2D;
    viewInfo.format                          = format;
    viewInfo.subresourceRange.aspectMask     = aspectFlags;
    viewInfo.subresourceRange.baseMipLevel   = 0;
    viewInfo.subresourceRange.levelCount     = 1;
    viewInfo.subresourceRange.baseArrayLayer = 0;
    viewInfo.subresourceRange.layerCount     = 1;

    VkImageView imageView;
    VK_CALL(vkCreateImageView(device, &viewInfo, nullptr, &imageView));

    return imageView;
}

void Vulk::recreateSwapChain() {
    WindowDims windowDims = {};
    glfwGetFramebufferSize(window, &windowDims.width, &windowDims.height);
    while (windowDims.width == 0 || windowDims.height == 0) {
        glfwGetFramebufferSize(window, &windowDims.width, &windowDims.height);
        glfwWaitEvents();
    }

    vkDeviceWaitIdle(device);

    cleanupSwapChain();

    createSwapChain();
    createImageViews();
    createDepthResources();
    createFramebuffers();
}

void Vulk::createInstance() {
    if (enableValidationLayers && !checkValidationLayerSupport()) {
        VULK_THROW("validation layers requested, but not available!");
    }

    VkApplicationInfo appInfo{};
    appInfo.sType              = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    appInfo.pApplicationName   = "VulkAppName";
    appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.pEngineName        = "VulkEngineName";
    appInfo.engineVersion      = VK_MAKE_VERSION(1, 0, 0);
    appInfo.apiVersion         = VK_API_VERSION_1_3;

    VkInstanceCreateInfo createInfo{};
    createInfo.sType            = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    createInfo.pApplicationInfo = &appInfo;

    auto extensions                    = getRequiredExtensions();
    createInfo.enabledExtensionCount   = static_cast<uint32_t>(extensions.size());
    createInfo.ppEnabledExtensionNames = extensions.data();

    VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo{};
    if (enableValidationLayers) {
        createInfo.enabledLayerCount   = static_cast<uint32_t>(validationLayers.size());
        createInfo.ppEnabledLayerNames = validationLayers.data();

        populateDebugMessengerCreateInfo(debugCreateInfo);
        createInfo.pNext = (VkDebugUtilsMessengerCreateInfoEXT*)&debugCreateInfo;
    } else {
        createInfo.enabledLayerCount = 0;

        createInfo.pNext = nullptr;
    }

    VK_CALL(vkCreateInstance(&createInfo, nullptr, &instance));
}

void Vulk::populateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& createInfo) {
    createInfo                 = {};
    createInfo.sType           = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
    createInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT |
                                 VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
    createInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
                             VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
    createInfo.pfnUserCallback = debugCallback;
}

void Vulk::setupDebug() {
    if (!enableValidationLayers)
        return;

    VkDebugUtilsMessengerCreateInfoEXT createInfo;
    populateDebugMessengerCreateInfo(createInfo);

    VK_CALL(CreateDebugUtilsMessengerEXT(instance, &createInfo, nullptr, &debugMessenger));

    pfnVkCmdBeginDebugUtilsLabelEXT =
        reinterpret_cast<PFN_vkCmdBeginDebugUtilsLabelEXT>(vkGetInstanceProcAddr(instance, "vkCmdBeginDebugUtilsLabelEXT"));
    pfnVkCmdEndDebugUtilsLabelEXT =
        reinterpret_cast<PFN_vkCmdEndDebugUtilsLabelEXT>(vkGetInstanceProcAddr(instance, "vkCmdEndDebugUtilsLabelEXT"));
    pfnVkCmdInsertDebugUtilsLabelEXT =
        reinterpret_cast<PFN_vkCmdInsertDebugUtilsLabelEXT>(vkGetInstanceProcAddr(instance, "vkCmdInsertDebugUtilsLabelEXT"));
}

void Vulk::createSurface() {
    VK_CALL(glfwCreateWindowSurface(instance, window, nullptr, &surface));
}

bool Vulk::isDeviceSuitable(VkPhysicalDevice physDevice) {
    QueueFamilyIndices inds = findQueueFamilies(physDevice, surface);

    bool extensionsSupported = checkDeviceExtensionSupport(physDevice);

    bool swapChainAdequate = false;
    if (extensionsSupported) {
        SwapChainSupportDetails swapChainSupport = querySwapChainSupport(physDevice, surface);
        swapChainAdequate                        = !swapChainSupport.formats.empty() && !swapChainSupport.presentModes.empty();
    }

    VkPhysicalDeviceFeatures supportedFeatures;
    vkGetPhysicalDeviceFeatures(physDevice, &supportedFeatures);

    return inds.isComplete() && extensionsSupported && swapChainAdequate && supportedFeatures.samplerAnisotropy;
}

bool Vulk::checkDeviceExtensionSupport(VkPhysicalDevice physDevice) {
    uint32_t extensionCount;
    vkEnumerateDeviceExtensionProperties(physDevice, nullptr, &extensionCount, nullptr);

    std::vector<VkExtensionProperties> availableExtensions(extensionCount);
    vkEnumerateDeviceExtensionProperties(physDevice, nullptr, &extensionCount, availableExtensions.data());

    std::set<std::string> requiredExtensions(deviceExtensions.begin(), deviceExtensions.end());

    for (const auto& extension : availableExtensions) {
        requiredExtensions.erase(extension.extensionName);
    }

    return requiredExtensions.empty();
}

void Vulk::pickPhysicalDevice() {
    uint32_t deviceCount = 0;
    VK_CALL(vkEnumeratePhysicalDevices(instance, &deviceCount, nullptr));

    if (deviceCount == 0) {
        VULK_THROW("failed to find GPUs with Vulkan support!");
    }

    std::vector<VkPhysicalDevice> devices(deviceCount);
    VK_CALL(vkEnumeratePhysicalDevices(instance, &deviceCount, devices.data()));

    for (const auto& d : devices) {
        if (isDeviceSuitable(d)) {
            physicalDevice = d;
            break;
        }
    }

    if (physicalDevice == VK_NULL_HANDLE) {
        VULK_THROW("failed to find a suitable GPU!");
    }

    debugPrintSupportedImageFormats();
}

void Vulk::createLogicalDevice() {
    indices = findQueueFamilies(physicalDevice, surface);

    std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
    std::set<uint32_t> uniqueQueueFamilies = {indices.graphicsFamily.value(), indices.presentFamily.value()};

    float queuePriority = 1.0f;
    for (uint32_t queueFamily : uniqueQueueFamilies) {
        VkDeviceQueueCreateInfo queueCreateInfo{};
        queueCreateInfo.sType            = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        queueCreateInfo.queueFamilyIndex = queueFamily;
        queueCreateInfo.queueCount       = 1;
        queueCreateInfo.pQueuePriorities = &queuePriority;
        queueCreateInfos.push_back(queueCreateInfo);
    }

    VkPhysicalDeviceFeatures deviceFeatures{};
    deviceFeatures.samplerAnisotropy = VK_TRUE;
    deviceFeatures.geometryShader    = VK_TRUE;
    deviceFeatures.fillModeNonSolid  = VK_TRUE;  // enables wireframe

    VkDeviceCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;

    createInfo.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size());
    createInfo.pQueueCreateInfos    = queueCreateInfos.data();

    createInfo.pEnabledFeatures = &deviceFeatures;

    createInfo.enabledExtensionCount   = static_cast<uint32_t>(deviceExtensions.size());
    createInfo.ppEnabledExtensionNames = deviceExtensions.data();

    if (enableValidationLayers) {
        createInfo.enabledLayerCount   = static_cast<uint32_t>(validationLayers.size());
        createInfo.ppEnabledLayerNames = validationLayers.data();
    } else {
        createInfo.enabledLayerCount = 0;
    }

    VK_CALL(vkCreateDevice(physicalDevice, &createInfo, nullptr, &device));

    vkGetDeviceQueue(device, indices.graphicsFamily.value(), 0, &graphicsQueue);
    vkGetDeviceQueue(device, indices.presentFamily.value(), 0, &presentQueue);
}

void Vulk::createSwapChain() {
    SwapChainSupportDetails swapChainSupport = querySwapChainSupport(physicalDevice, surface);

    surfaceFormat     = chooseSwapSurfaceFormat(swapChainSupport.formats);
    presentMode       = chooseSwapPresentMode(swapChainSupport.presentModes);
    VkExtent2D extent = chooseSwapExtent(swapChainSupport.capabilities);

    uint32_t imageCount = swapChainSupport.capabilities.minImageCount + 1;  // why +1?
    if (swapChainSupport.capabilities.maxImageCount > 0 && imageCount > swapChainSupport.capabilities.maxImageCount) {
        imageCount = swapChainSupport.capabilities.maxImageCount;
    }

    VkSwapchainCreateInfoKHR createInfo{};
    createInfo.sType   = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    createInfo.surface = surface;

    createInfo.minImageCount    = imageCount;
    createInfo.imageFormat      = surfaceFormat.format;
    createInfo.imageColorSpace  = surfaceFormat.colorSpace;
    createInfo.imageExtent      = extent;
    createInfo.imageArrayLayers = 1;
    createInfo.imageUsage       = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

    uint32_t queueFamilyIndices[] = {indices.graphicsFamily.value(), indices.presentFamily.value()};

    if (indices.graphicsFamily != indices.presentFamily) {
        createInfo.imageSharingMode      = VK_SHARING_MODE_CONCURRENT;
        createInfo.queueFamilyIndexCount = 2;
        createInfo.pQueueFamilyIndices   = queueFamilyIndices;
    } else {
        createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
    }

    createInfo.preTransform   = swapChainSupport.capabilities.currentTransform;
    createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    createInfo.presentMode    = presentMode;
    createInfo.clipped        = VK_TRUE;

    VK_CALL(vkCreateSwapchainKHR(device, &createInfo, nullptr, &swapChain));

    vkGetSwapchainImagesKHR(device, swapChain, &imageCount, nullptr);
    swapChainImages.resize(imageCount);
    vkGetSwapchainImagesKHR(device, swapChain, &imageCount, swapChainImages.data());

    swapChainImageFormat = surfaceFormat.format;
    swapChainExtent      = extent;
}

void Vulk::createImageViews() {
    swapChainImageViews.resize(swapChainImages.size());

    for (uint32_t i = 0; i < swapChainImages.size(); i++) {
        swapChainImageViews[i] = createImageView(swapChainImages[i], swapChainImageFormat, VK_IMAGE_ASPECT_COLOR_BIT);
    }
}

void Vulk::createRenderPass() {
    VkAttachmentDescription colorAttachment{};
    colorAttachment.format         = swapChainImageFormat;
    colorAttachment.samples        = VK_SAMPLE_COUNT_1_BIT;
    colorAttachment.loadOp         = VK_ATTACHMENT_LOAD_OP_CLEAR;
    colorAttachment.storeOp        = VK_ATTACHMENT_STORE_OP_STORE;
    colorAttachment.stencilLoadOp  = VK_ATTACHMENT_LOAD_OP_CLEAR;
    colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    colorAttachment.initialLayout  = VK_IMAGE_LAYOUT_UNDEFINED;
    colorAttachment.finalLayout    = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

    VkAttachmentDescription depthAttachment{};
    depthAttachment.format         = findDepthFormat();
    depthAttachment.samples        = VK_SAMPLE_COUNT_1_BIT;
    depthAttachment.loadOp         = VK_ATTACHMENT_LOAD_OP_CLEAR;
    depthAttachment.storeOp        = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    depthAttachment.stencilLoadOp  = VK_ATTACHMENT_LOAD_OP_CLEAR;
    depthAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    depthAttachment.initialLayout  = VK_IMAGE_LAYOUT_UNDEFINED;
    depthAttachment.finalLayout    = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    VkAttachmentReference colorAttachmentRef{};
    colorAttachmentRef.attachment = 0;
    colorAttachmentRef.layout     = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    VkAttachmentReference depthAttachmentRef{};
    depthAttachmentRef.attachment = 1;
    depthAttachmentRef.layout     = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    VkSubpassDescription subpass{};
    subpass.pipelineBindPoint       = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.colorAttachmentCount    = 1;
    subpass.pColorAttachments       = &colorAttachmentRef;
    subpass.pDepthStencilAttachment = &depthAttachmentRef;

    VkSubpassDependency dependency{};
    dependency.srcSubpass    = VK_SUBPASS_EXTERNAL;
    dependency.dstSubpass    = 0;
    dependency.srcStageMask  = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
    dependency.srcAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
    dependency.dstStageMask  = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
    dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

    std::array<VkAttachmentDescription, 2> attachments = {colorAttachment, depthAttachment};
    VkRenderPassCreateInfo renderPassInfo{};
    renderPassInfo.sType           = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    renderPassInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
    renderPassInfo.pAttachments    = attachments.data();
    renderPassInfo.subpassCount    = 1;
    renderPassInfo.pSubpasses      = &subpass;
    renderPassInfo.dependencyCount = 1;
    renderPassInfo.pDependencies   = &dependency;

    VK_CALL(vkCreateRenderPass(device, &renderPassInfo, nullptr, &renderPass));
}

void Vulk::createFramebuffers() {
    swapChainFramebuffers.resize(swapChainImageViews.size());

    for (size_t i = 0; i < swapChainImageViews.size(); i++) {
        std::array<VkImageView, 2> attachments = {swapChainImageViews[i], depthImageView};

        VkFramebufferCreateInfo framebufferInfo{};
        framebufferInfo.sType           = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        framebufferInfo.renderPass      = renderPass;
        framebufferInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
        framebufferInfo.pAttachments    = attachments.data();
        framebufferInfo.width           = swapChainExtent.width;
        framebufferInfo.height          = swapChainExtent.height;
        framebufferInfo.layers          = 1;

        VK_CALL(vkCreateFramebuffer(device, &framebufferInfo, nullptr, &swapChainFramebuffers[i]));
    }
}

void Vulk::createCommandPool() {
    QueueFamilyIndices queueFamilyIndices = findQueueFamilies(physicalDevice, surface);

    VkCommandPoolCreateInfo poolInfo{};
    poolInfo.sType            = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    poolInfo.flags            = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    poolInfo.queueFamilyIndex = queueFamilyIndices.graphicsFamily.value();

    VK_CALL(vkCreateCommandPool(device, &poolInfo, nullptr, &commandPool));
}

void Vulk::createCommandBuffers() {
    commandBuffers.resize(MAX_FRAMES_IN_FLIGHT);

    VkCommandBufferAllocateInfo allocInfo{};
    allocInfo.sType              = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.commandPool        = commandPool;
    allocInfo.level              = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandBufferCount = (uint32_t)commandBuffers.size();

    VK_CALL(vkAllocateCommandBuffers(device, &allocInfo, commandBuffers.data()));
}

void Vulk::createDepthResources() {
    VkFormat depthFormat = findDepthFormat();

    createImage(swapChainExtent.width,
                swapChainExtent.height,
                depthFormat,
                VK_IMAGE_TILING_OPTIMAL,
                VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
                VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                depthImage,
                depthImageMemory);
    depthImageView = createImageView(depthImage, depthFormat, VK_IMAGE_ASPECT_DEPTH_BIT);
}

VkFormat Vulk::findSupportedFormat(const std::vector<VkFormat>& candidates, VkImageTiling tiling, VkFormatFeatureFlags features) {
    for (VkFormat format : candidates) {
        VkFormatProperties props;
        vkGetPhysicalDeviceFormatProperties(physicalDevice, format, &props);

        if (tiling == VK_IMAGE_TILING_LINEAR && (props.linearTilingFeatures & features) == features) {
            return format;
        } else if (tiling == VK_IMAGE_TILING_OPTIMAL && (props.optimalTilingFeatures & features) == features) {
            return format;
        }
    }

    VULK_THROW("failed to find supported format!");
}

VkFormat Vulk::findDepthFormat() {
    return findSupportedFormat(
        {
            // VK_FORMAT_D32_SFLOAT : we only want stencil supported buffer, not
            // just depth buffer
            VK_FORMAT_D32_SFLOAT_S8_UINT,
            VK_FORMAT_D24_UNORM_S8_UINT,
        },
        VK_IMAGE_TILING_OPTIMAL,
        VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT);
}

bool Vulk::hasStencilComponent(VkFormat format) {
    return format == VK_FORMAT_D32_SFLOAT_S8_UINT || format == VK_FORMAT_D24_UNORM_S8_UINT;
}

std::unique_ptr<VkImageFormatProperties2> Vulk::getDeviceImageFormatProperties(VkFormat format,
                                                                               VkImageTiling tiling,
                                                                               VkImageUsageFlags usage) {
    // VkImageFormatProperties2 ifp2 = {};
    std::unique_ptr<VkImageFormatProperties2> ifp2 = std::make_unique<VkImageFormatProperties2>();
    ifp2->sType                                    = VK_STRUCTURE_TYPE_IMAGE_FORMAT_PROPERTIES_2;
    ifp2->pNext                                    = nullptr;
    VkPhysicalDeviceImageFormatInfo2 pdifi2        = {};
    pdifi2.sType                                   = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_IMAGE_FORMAT_INFO_2;
    pdifi2.format                                  = format;
    pdifi2.tiling                                  = tiling;
    pdifi2.type                                    = VK_IMAGE_TYPE_2D;
    pdifi2.usage                                   = usage;
    if (VK_SUCCESS == vkGetPhysicalDeviceImageFormatProperties2(physicalDevice, &pdifi2, ifp2.get())) {
        return ifp2;
    }
    return nullptr;
}

void Vulk::createImage(uint32_t width,
                       uint32_t height,
                       VkFormat format,
                       VkImageTiling tiling,
                       VkImageUsageFlags usage,
                       VkMemoryPropertyFlags properties,
                       VkImage& image,
                       VkDeviceMemory& imageMemory) {
    VkImageCreateInfo imageInfo{};
    imageInfo.sType         = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    imageInfo.imageType     = VK_IMAGE_TYPE_2D;
    imageInfo.extent.width  = width;
    imageInfo.extent.height = height;
    imageInfo.extent.depth  = 1;
    imageInfo.mipLevels     = 1;
    imageInfo.arrayLayers   = 1;
    imageInfo.format        = format;
    imageInfo.tiling        = tiling;
    imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    imageInfo.usage         = usage;
    imageInfo.samples       = VK_SAMPLE_COUNT_1_BIT;
    imageInfo.sharingMode   = VK_SHARING_MODE_EXCLUSIVE;
    imageInfo.flags         = 0;  // Optional

    VK_CALL(vkCreateImage(device, &imageInfo, nullptr, &image));

    VkMemoryRequirements memRequirements;
    vkGetImageMemoryRequirements(device, image, &memRequirements);

    VkMemoryAllocateInfo allocInfo{};
    allocInfo.sType           = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize  = memRequirements.size;
    allocInfo.memoryTypeIndex = findMemoryType(memRequirements.memoryTypeBits, properties);

    VK_CALL(vkAllocateMemory(device, &allocInfo, nullptr, &imageMemory));
    VK_CALL(vkBindImageMemory(device, image, imageMemory, 0));
}

void Vulk::transitionImageLayout(VkCommandBuffer commandBuffer,
                                 VkImage image,
                                 VkImageLayout oldLayout,
                                 VkImageLayout newLayout,
                                 uint32_t mipLevels,
                                 uint32_t layerCount) {
    VkImageMemoryBarrier barrier{};
    barrier.sType               = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    barrier.oldLayout           = oldLayout;
    barrier.newLayout           = newLayout;
    barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.image               = image;

    VkPipelineStageFlags sourceStage;
    VkPipelineStageFlags destinationStage;
    VkImageAspectFlags aspectMask;
    if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL) {
        barrier.srcAccessMask = 0;
        barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

        sourceStage      = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
        destinationStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
        aspectMask       = VK_IMAGE_ASPECT_COLOR_BIT;
    } else if (oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
        barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

        sourceStage      = VK_PIPELINE_STAGE_TRANSFER_BIT;
        destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
        aspectMask       = VK_IMAGE_ASPECT_COLOR_BIT;
    } else if (oldLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL &&
               newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
        barrier.srcAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
        barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

        sourceStage      = VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
        destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
        aspectMask       = VK_IMAGE_ASPECT_DEPTH_BIT;  // | VK_IMAGE_ASPECT_STENCIL_BIT;
    } else if (oldLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL &&
               newLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL) {
        barrier.srcAccessMask = VK_ACCESS_SHADER_READ_BIT;
        barrier.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

        sourceStage      = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
        destinationStage = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
        aspectMask       = VK_IMAGE_ASPECT_DEPTH_BIT;  // | VK_IMAGE_ASPECT_STENCIL_BIT;
    } else if (oldLayout == VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL) {
        barrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
        barrier.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

        sourceStage      = VK_PIPELINE_STAGE_TRANSFER_BIT;
        destinationStage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        aspectMask       = VK_IMAGE_ASPECT_COLOR_BIT;
    } else {
        VULK_THROW("unsupported layout transition : {} -> {}", std::to_string(oldLayout), std::to_string(newLayout));
    }

    barrier.subresourceRange.aspectMask     = aspectMask;
    barrier.subresourceRange.baseMipLevel   = 0;
    barrier.subresourceRange.levelCount     = mipLevels;
    barrier.subresourceRange.baseArrayLayer = 0;
    barrier.subresourceRange.layerCount     = layerCount;

    vkCmdPipelineBarrier(commandBuffer, sourceStage, destinationStage, 0, 0, nullptr, 0, nullptr, 1, &barrier);
}

void Vulk::copyBufferToImage(VkBuffer buffer, VkImage image, uint32_t width, uint32_t height) {
    VkCommandBuffer commandBuffer = beginSingleTimeCommands();

    VkBufferImageCopy region{};
    region.bufferOffset                    = 0;
    region.bufferRowLength                 = 0;
    region.bufferImageHeight               = 0;
    region.imageSubresource.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
    region.imageSubresource.mipLevel       = 0;
    region.imageSubresource.baseArrayLayer = 0;
    region.imageSubresource.layerCount     = 1;
    region.imageOffset                     = {0, 0, 0};
    region.imageExtent                     = {width, height, 1};

    vkCmdCopyBufferToImage(commandBuffer, buffer, image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);

    endSingleTimeCommands(commandBuffer);
}

VkCommandBuffer Vulk::beginSingleTimeCommands() {
    VkCommandBufferAllocateInfo allocInfo{};
    allocInfo.sType              = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.level              = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandPool        = commandPool;
    allocInfo.commandBufferCount = 1;

    VkCommandBuffer commandBuffer;
    VK_CALL(vkAllocateCommandBuffers(device, &allocInfo, &commandBuffer));

    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

    VK_CALL(vkBeginCommandBuffer(commandBuffer, &beginInfo));

    return commandBuffer;
}

void Vulk::endSingleTimeCommands(VkCommandBuffer commandBuffer) {
    VK_CALL(vkEndCommandBuffer(commandBuffer));

    VkSubmitInfo submitInfo{};
    submitInfo.sType              = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers    = &commandBuffer;

    VK_CALL(vkQueueSubmit(graphicsQueue, 1, &submitInfo, VK_NULL_HANDLE));
    VK_CALL(vkQueueWaitIdle(graphicsQueue));

    vkFreeCommandBuffers(device, commandPool, 1, &commandBuffer);
}

void Vulk::createSyncObjects() {
    imageAvailableSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
    renderFinishedSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
    inFlightFences.resize(MAX_FRAMES_IN_FLIGHT);

    VkSemaphoreCreateInfo semaphoreInfo{};
    semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

    VkFenceCreateInfo fenceInfo{};
    fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
        if (vkCreateSemaphore(device, &semaphoreInfo, nullptr, &imageAvailableSemaphores[i]) != VK_SUCCESS ||
            vkCreateSemaphore(device, &semaphoreInfo, nullptr, &renderFinishedSemaphores[i]) != VK_SUCCESS ||
            vkCreateFence(device, &fenceInfo, nullptr, &inFlightFences[i]) != VK_SUCCESS) {
            VULK_THROW("failed to create synchronization objects for a frame!");
        }
    }
}

void Vulk::render() {
    VK_CALL(vkWaitForFences(device, 1, &inFlightFences[currentFrame], VK_TRUE, UINT64_MAX));

    if (renderable && lastFrame < MAX_FRAMES_IN_FLIGHT)
        renderable->onBeforeRender();

    VkResult result = vkAcquireNextImageKHR(device,
                                            swapChain,
                                            UINT64_MAX,
                                            imageAvailableSemaphores[currentFrame],
                                            VK_NULL_HANDLE,
                                            &swapChainImageIndex);

    if (result == VK_ERROR_OUT_OF_DATE_KHR) {
        recreateSwapChain();
        return;
    } else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
        VULK_THROW("failed to acquire swap chain image!");
    }

    // updateUniformBuffer(currentFrame); AB: moved to derived class, but leaving
    // here as this position might be important as a reminder

    VK_CALL(vkResetFences(device, 1, &inFlightFences[currentFrame]));

    VkCommandBuffer commandBuffer = commandBuffers[currentFrame];
    VK_CALL(vkResetCommandBuffer(commandBuffer, /*VkCommandBufferResetFlagBits*/ 0));

    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    VK_CALL(vkBeginCommandBuffer(commandBuffer, &beginInfo));

    if (renderable) {
        renderable->renderFrame(commandBuffer, swapChainImageIndex);
    }

    if (uiRenderer) {
        uiRenderer->renderFrame(commandBuffer, swapChainImageIndex);
    }

    VK_CALL(vkEndCommandBuffer(commandBuffer));

    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

    VkSemaphore waitSemaphores[]      = {imageAvailableSemaphores[currentFrame]};
    VkPipelineStageFlags waitStages[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
    submitInfo.waitSemaphoreCount     = 1;
    submitInfo.pWaitSemaphores        = waitSemaphores;
    submitInfo.pWaitDstStageMask      = waitStages;

    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers    = &commandBuffer;

    VkSemaphore signalSemaphores[]  = {renderFinishedSemaphores[currentFrame]};
    submitInfo.signalSemaphoreCount = 1;
    submitInfo.pSignalSemaphores    = signalSemaphores;

    VK_CALL(vkQueueSubmit(graphicsQueue, 1, &submitInfo, inFlightFences[currentFrame]));

    VkPresentInfoKHR presentInfo{};
    presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;

    presentInfo.waitSemaphoreCount = 1;
    presentInfo.pWaitSemaphores    = signalSemaphores;

    VkSwapchainKHR swapChains[] = {swapChain};
    presentInfo.swapchainCount  = 1;
    presentInfo.pSwapchains     = swapChains;

    presentInfo.pImageIndices = &swapChainImageIndex;

    result = vkQueuePresentKHR(presentQueue, &presentInfo);

    if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR || framebufferResized) {
        framebufferResized = false;
        recreateSwapChain();
    } else if (result != VK_SUCCESS) {
        VULK_THROW("failed to present swap chain image!");
    }

    if (renderable)
        renderable->onAfterPresent();

    lastFrame    = currentFrame;
    currentFrame = (currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;
    frameCount++;
}

VkSurfaceFormatKHR Vulk::chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats) {
    VULK_ASSERT(!availableFormats.empty(), "No available surface formats");
    // Iterate over available formats and choose the desired one
    for (const auto& sf : availableFormats) {
        if (sf.format == VK_FORMAT_B8G8R8A8_SRGB && sf.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
            return sf;  // Found the desired format
        }
    }

    // Fallback to the first available format
    // This is a reasonable choice, but you might want to add a comment explaining why
    logger->warn("Desired surface format not found, falling back to first available format");
    return availableFormats[0];
}
VkPresentModeKHR Vulk::chooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes) {
    for (const auto& availablePresentMode : availablePresentModes) {
        if (availablePresentMode == VK_PRESENT_MODE_MAILBOX_KHR) {
            return availablePresentMode;
        }
    }

    return VK_PRESENT_MODE_FIFO_KHR;
}

VkExtent2D Vulk::chooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities) {
    if (capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max()) {
        return capabilities.currentExtent;
    } else {
        WindowDims windowDims = {};
        glfwGetFramebufferSize(window, &windowDims.width, &windowDims.height);
        VkExtent2D actualExtent = {static_cast<uint32_t>(windowDims.width), static_cast<uint32_t>(windowDims.height)};

        actualExtent.width = std::clamp(actualExtent.width, capabilities.minImageExtent.width, capabilities.maxImageExtent.width);
        actualExtent.height =
            std::clamp(actualExtent.height, capabilities.minImageExtent.height, capabilities.maxImageExtent.height);

        return actualExtent;
    }
}

std::vector<const char*> Vulk::getRequiredExtensions() {
    uint32_t glfwExtensionCount = 0;
    const char** glfwExtensions;
    glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

    std::vector<const char*> extensions(glfwExtensions, glfwExtensions + glfwExtensionCount);

    if (enableValidationLayers) {
        extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
    }

    return extensions;
}

bool Vulk::checkValidationLayerSupport() {
    uint32_t layerCount;
    vkEnumerateInstanceLayerProperties(&layerCount, nullptr);

    std::vector<VkLayerProperties> availableLayers(layerCount);
    vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());

    for (const char* layerName : validationLayers) {
        bool layerFound = false;

        for (const auto& layerProperties : availableLayers) {
            if (strcmp(layerName, layerProperties.layerName) == 0) {
                layerFound = true;
                break;
            }
        }

        if (!layerFound) {
            return false;
        }
    }

    return true;
}

VKAPI_ATTR VkBool32 VKAPI_CALL Vulk::debugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
                                                   VkDebugUtilsMessageTypeFlagsEXT messageType,
                                                   const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
                                                   void* /*pUserData*/
) {
    char const* severity;
    if (messageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT) {
        severity = "ERROR: ";
    } else if (messageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT) {
        severity = "WARNING: ";
    } else if (messageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT) {
        severity = "INFO: ";
    } else if (messageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT) {
        severity = "VERBOSE: ";
    } else {
        severity = "UNKNOWN: ";
    }
    std::cerr << "Vulk: " << severity << std::hex << messageType << " message: " << pCallbackData->pMessage << std::endl;
    if ((messageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT)
        // super annoying: I used vulkan configurator to see if it did anything
        // useful and now I can't figure out how to turn this off.
        && 0 != strcmp(pCallbackData->pMessage,
                       "loader_get_json: Failed to open JSON file C:\\Program "
                       "Files\\IntelSWTools\\GPA\\Streams\\VkLayer_state_tracker."
                       "json")) {
        VULK_THROW("validation layer error");
    }

    return VK_FALSE;
}

VkResult Vulk::CreateDebugUtilsMessengerEXT(VkInstance instance,
                                            const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo,
                                            const VkAllocationCallbacks* pAllocator,
                                            VkDebugUtilsMessengerEXT* pDebugMessenger) {
    auto func = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT");
    if (func != nullptr) {
        return func(instance, pCreateInfo, pAllocator, pDebugMessenger);
    } else {
        return VK_ERROR_EXTENSION_NOT_PRESENT;
    }
}

void Vulk::DestroyDebugUtilsMessengerEXT(VkInstance instance,
                                         VkDebugUtilsMessengerEXT debugMessenger,
                                         const VkAllocationCallbacks* pAllocator) {
    auto func = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT");
    if (func != nullptr) {
        func(instance, debugMessenger, pAllocator);
    }
}

void Vulk::dispatchKeyCallback(GLFWwindow* window, int key, int scancode, int action, int mods) {
    // Retrieve the user pointer and use it to call the member function
    Vulk* self = static_cast<Vulk*>(glfwGetWindowUserPointer(window));
    self->keyCallback(key, scancode, action, mods);
}

void Vulk::keyCallback(int key, int /*scancode*/, int action, int /*mods*/) {
    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS) {
        glfwSetWindowShouldClose(window, GLFW_TRUE);
    }
}

void Vulk::debugPrintSupportedImageFormats() {
    std::vector<std::pair<VkFormat, std::string>> imageFormats = {
        {VK_FORMAT_R8G8B8A8_UNORM, "VK_FORMAT_R8G8B8A8_UNORM"},
        {VK_FORMAT_R8G8B8A8_SRGB, "VK_FORMAT_R8G8B8A8_SRGB"},
        {VK_FORMAT_R8G8B8A8_SNORM, "VK_FORMAT_R8G8B8A8_SNORM"},
        {VK_FORMAT_R8G8B8A8_UINT, "VK_FORMAT_R8G8B8A8_UINT"},
        {VK_FORMAT_R8G8B8A8_SINT, "VK_FORMAT_R8G8B8A8_SINT"},
        // {VK_FORMAT_R16G16B16_SFLOAT, "VK_FORMAT_R16G16B16_SFLOAT"},
        {VK_FORMAT_R16G16_SFLOAT, "VK_FORMAT_R16G16_SFLOAT"},
        {VK_FORMAT_D32_SFLOAT, "VK_FORMAT_D32_SFLOAT"},
    };
    for (auto [format, name] : imageFormats) {
        auto ifp2 = getDeviceImageFormatProperties(format, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_TRANSFER_DST_BIT);
        if (ifp2) {
            logger->info("Supported image format: {}", name);
        } else {
            logger->info("Unsupported image format: {}", name);
        }
    }
}

void Vulk::beginDebugLabel(VkCommandBuffer cmdbuffer, std::string caption, glm::vec4 color) {
    if (!pfnVkCmdBeginDebugUtilsLabelEXT) {
        return;
    }
    VkDebugUtilsLabelEXT labelInfo{};
    labelInfo.sType      = VK_STRUCTURE_TYPE_DEBUG_UTILS_LABEL_EXT;
    labelInfo.pLabelName = caption.c_str();
    memcpy(labelInfo.color, &color[0], sizeof(float) * 4);
    pfnVkCmdBeginDebugUtilsLabelEXT(cmdbuffer, &labelInfo);
}

void Vulk::endDebugLabel(VkCommandBuffer cmdbuffer) {
    if (!pfnVkCmdEndDebugUtilsLabelEXT) {
        return;
    }
    pfnVkCmdEndDebugUtilsLabelEXT(cmdbuffer);
}

void Vulk::insertDebugLabel(VkCommandBuffer cmdBuffer, std::string caption, glm::vec4 color) {
    if (!pfnVkCmdInsertDebugUtilsLabelEXT) {
        return;
    }
    VkDebugUtilsLabelEXT label = {
        .sType      = VK_STRUCTURE_TYPE_DEBUG_UTILS_LABEL_EXT,
        .pLabelName = caption.c_str(),
        .color      = {color[0], color[1], color[2], color[3]},
    };
    pfnVkCmdInsertDebugUtilsLabelEXT(cmdBuffer, &label);
}