#ifndef VULK_INCLUDE_H
#define VULK_INCLUDE_H

#pragma once

#include "VulkUtil.h"
#include <algorithm>
#include <array>
#include <chrono>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <iostream>
#include <limits>
#include <mutex>
#include <optional>
#include <set>
#include <stdexcept>
#include <thread>
#include <unordered_map>
#include <vector>

struct MouseDragContext {
    double dt = 0.0;
    double dxdt = 0.0;
    double dydt = 0.0;
    double dragStartX = 0.0;
    double dragStartY = 0.0;
};

struct MouseEventContext {
    bool isDragging = false;
    double lastClickTime = 0.0;
    double lastCursorPosTime = 0.0;
    double lastX = 0.0, lastY = 0.0;
    bool shift = false;
    bool control = false;
    bool alt = false;
};

#pragma warning(push)
#pragma warning(disable : 4100)
class MouseEventHandler {
public:
    virtual void onClick(double xpos, double ypos, MouseEventContext const& ctxt) {}
    virtual void onDoubleClick(double xpos, double ypos, MouseEventContext const& ctxt) {}
    virtual void onMouseDown(double xpos, double ypos, MouseEventContext const& ctxt) {}
    virtual void onMouseMove(double xpos, double ypos, MouseEventContext const& ctxt) {}
    virtual void onMouseUp(double xpos, double ypos, MouseEventContext const& ctxt) {}
    virtual void onDrag(double xpos, double ypos, MouseDragContext const& drag, MouseEventContext const& ctxt) {}
    virtual void onDragStart(double xpos, double ypos, MouseEventContext const& ctxt) {}
    virtual void onDragEnd(double xpos, double ypos, MouseEventContext const& ctxt) {}
};
#pragma warning(pop)

void setMouseEventHandler(MouseEventHandler* handler);
void clearMouseEventHandler();

// TODO: constexpr?
const int MAX_FRAMES_IN_FLIGHT = 2;
const uint32_t WINDOW_WIDTH = 2880;
const uint32_t WINDOW_HEIGHT = 1800;

enum VulkTextureType {
    VulkTextureType_Diffuse,
    VulkTextureType_Normal,
    VulkTextureType_Specular,
    VulkTextureType_Emissive,
    VulkTextureType_AmbientOcclusion,
    VulkTextureType_Roughness,
    VulkTextureType_Metallic,
    VulkTextureType_Height,
    VulkTextureType_MaxTextureTypes
};

class VulkRenderable {
public:
    virtual ~VulkRenderable() = default;
    virtual void tick() = 0;
    virtual void renderFrame(VkCommandBuffer commandBuffer, VkFramebuffer frameBuffer) = 0;

    // ==================
    // events

    // after vkQueuePresentKHR but before currentFrame is incremented
    virtual void onAfterPresent() {}
    // after vkWaitForFences, before vkBeginCommandBuffer
    virtual void onBeforeRender() {}
};

class VulkImGui;

using namespace std::chrono_literals; // allows things like 16ms

class Vulk {
    std::chrono::time_point<std::chrono::steady_clock> lastFrameTime;
    std::chrono::milliseconds msPerFrame = 16ms; // 60 fps
public:
    // TODO: this is just a mess
    std::shared_ptr<VulkRenderable> renderable;
    std::shared_ptr<VulkImGui> uiRenderer;

    Vulk();

    void run();

public:
    VkDevice device;
    VkRenderPass renderPass;
    VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;
    VkPresentModeKHR presentMode; // for ImGUI

public: // utilities
    void createBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer& buffer, VkDeviceMemory& bufferMemory);
    void copyMemToBuffer(void const* srcBuffer, VkBuffer dstBuffer, VkDeviceSize size);
    void copyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size);
    void copyImageToBuffer(VkImage image, VkBuffer buffer, uint32_t width, uint32_t height);
    void copyBufferToMem(VkBuffer srcBuffer, void* dstMem, VkDeviceSize size);
    void copyImageToMem(VkImage image, void* dstBuffer, uint32_t width, uint32_t height, VkDeviceSize dstEltSize);
    VkSampler createTextureSampler();
    VkImageView createImageView(VkImage image, VkFormat format, VkImageAspectFlags aspectFlags);
    VkImage createTextureImage(char const* texture_path, VkDeviceMemory& textureImageMemory, VkImage& textureImage, bool isUNORM, VkFormat& formatOut);
    uint32_t findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties);
    VkShaderModule createShaderModule(const std::vector<char>& code);
    VkDescriptorSet createDescriptorSet(VkDescriptorSetLayout descriptorSetLayout, VkDescriptorPool descriptorPool);
    VkFormat findSupportedFormat(const std::vector<VkFormat>& candidates, VkImageTiling tiling, VkFormatFeatureFlags features);
    VkFormat findDepthFormat();
    void createImage(uint32_t width, uint32_t height, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage, VkMemoryPropertyFlags properties, VkImage& image,
                     VkDeviceMemory& imageMemory);

    // e.g. convert a created buffer to a texture buffer or when you transition a depth buffer to a shader readable format
    void transitionImageLayout(VkCommandBuffer commandBuffer, VkImage image, VkImageLayout oldLayout, VkImageLayout newLayout, uint32_t mipLevels = 1, uint32_t layerCount = 1);

    uint32_t currentFrame = 0; // index of the current frame in flight, always between 0 and MAX_FRAMES_IN_FLIGHT
    uint32_t lastFrame = UINT32_MAX;
    uint32_t frameCount = 0;
    VkExtent2D swapChainExtent;
    VkCommandPool commandPool;
    std::vector<VkCommandBuffer> commandBuffers;

private:
    std::function<void(GLFWwindow*, int, int, int)> onMouseButton;
    std::function<void(GLFWwindow*, double, double)> onCursorMove;

public:
    VkInstance instance;
    virtual void handleEvents() {
        // override this to call things like glfwGetKey and glfwGetMouseButton
    }
    virtual void keyCallback(int key, int /*scancode*/, int action, int /*mods*/);

public:
    bool enableValidationLayers = true;
    GLFWwindow* window;
    struct WindowDims {
        int width = 0, height = 0;
    } windowDims;

    QueueFamilyIndices indices;
    VkSurfaceFormatKHR surfaceFormat;

    VkDebugUtilsMessengerEXT debugMessenger;
    VkSurfaceKHR surface;

    VkQueue graphicsQueue;
    VkQueue presentQueue;

    VkSwapchainKHR swapChain;
    std::vector<VkImage> swapChainImages;
    VkFormat swapChainImageFormat;
    std::vector<VkImageView> swapChainImageViews;
    std::vector<VkFramebuffer> swapChainFramebuffers;

    std::vector<VkSemaphore> imageAvailableSemaphores;
    std::vector<VkSemaphore> renderFinishedSemaphores;
    std::vector<VkFence> inFlightFences;

    VkImage depthImage;
    VkDeviceMemory depthImageMemory;
    VkImageView depthImageView;

    bool framebufferResized = false;

    static void framebufferResizeCallback(GLFWwindow* window, int /*width*/, int /*height*/);

    VkCommandBuffer beginSingleTimeCommands();
    void endSingleTimeCommands(VkCommandBuffer commandBuffer);
    void copyBufferToImage(VkBuffer buffer, VkImage image, uint32_t width, uint32_t height);

private:
    void initWindow();
    void initVulkan();
    void cleanupSwapChain();
    void cleanupVulkan();
    void recreateSwapChain();
    void createInstance();
    void populateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& createInfo);
    void setupDebugMessenger();
    void createSurface();
    bool isDeviceSuitable(VkPhysicalDevice physDevice);
    bool checkDeviceExtensionSupport(VkPhysicalDevice physDevice);
    void pickPhysicalDevice();
    void createLogicalDevice();
    void createSwapChain();
    void createImageViews();
    void createRenderPass();
    void createFramebuffers();
    void createCommandPool();
    void createCommandBuffers();
    void createDepthResources();
    bool hasStencilComponent(VkFormat format);
    void createSyncObjects();
    void render();
    VkSurfaceFormatKHR chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats);
    VkPresentModeKHR chooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes);
    VkExtent2D chooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities);
    std::vector<const char*> getRequiredExtensions();
    bool checkValidationLayerSupport();
    static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity, VkDebugUtilsMessageTypeFlagsEXT messageType,
                                                        const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData, void* /*pUserData*/);
    static VkResult CreateDebugUtilsMessengerEXT(VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo, const VkAllocationCallbacks* pAllocator,
                                                 VkDebugUtilsMessengerEXT* pDebugMessenger);
    static void DestroyDebugUtilsMessengerEXT(VkInstance instance, VkDebugUtilsMessengerEXT debugMessenger, const VkAllocationCallbacks* pAllocator);
    static void dispatchKeyCallback(GLFWwindow* window, int key, int scancode, int action, int mods);
};

#endif // VULK_INCLUDE_H