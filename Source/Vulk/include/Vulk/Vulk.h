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

using namespace std::chrono_literals;

const int MAX_FRAMES_IN_FLIGHT = 2;

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

class Vulk {
    std::chrono::time_point<std::chrono::steady_clock> lastFrameTime;
    std::chrono::milliseconds msPerFrame = 16ms; // 60 fps
  public:
    void run();

  public:
    VkDevice device;
    VkRenderPass renderPass;
    VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;
    void createBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer &buffer, VkDeviceMemory &bufferMemory);
    void copyFromMemToBuffer(void const *srcBuffer, VkBuffer dstBuffer, VkDeviceSize size);
    void copyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size);
    VkSampler createTextureSampler();
    VkImageView createImageView(VkImage image, VkFormat format, VkImageAspectFlags aspectFlags);
    VkImage createTextureImage(char const *texture_path, VkDeviceMemory &textureImageMemory, VkImage &textureImage, bool isUNORM, VkFormat &formatOut);
    uint32_t findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties);
    VkShaderModule createShaderModule(const std::vector<char> &code);
    VkDescriptorSet createDescriptorSet(VkDescriptorSetLayout descriptorSetLayout, VkDescriptorPool descriptorPool);
    void setMouseHandler(std::function<void(GLFWwindow *, int, int, int)> onButton, std::function<void(GLFWwindow *, double, double)> onMove) {
        glfwSetMouseButtonCallback(window, [](GLFWwindow *w, int button, int action, int mods) {
            static_cast<Vulk *>(glfwGetWindowUserPointer(w))->onMouseButton(w, button, action, mods);
        });
        glfwSetCursorPosCallback(
            window, [](GLFWwindow *w, double xpos, double ypos) { static_cast<Vulk *>(glfwGetWindowUserPointer(w))->onCursorMove(w, xpos, ypos); });
        onMouseButton = onButton;
        onCursorMove = onMove;
        glfwSetWindowUserPointer(window, this);
    }

    VkFormat findSupportedFormat(const std::vector<VkFormat> &candidates, VkImageTiling tiling, VkFormatFeatureFlags features);
    VkFormat findDepthFormat();
    void createImage(uint32_t width, uint32_t height, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage, VkMemoryPropertyFlags properties,
                     VkImage &image, VkDeviceMemory &imageMemory);

    // e.g. convert a created buffer to a texture buffer or when you transition a depth buffer to a shader readable format
    void transitionImageLayout(VkCommandBuffer commandBuffer, VkImage image, VkImageLayout oldLayout, VkImageLayout newLayout);

    uint32_t currentFrame = 0; // index of the current frame in flight, always between 0 and MAX_FRAMES_IN_FLIGHT
    VkExtent2D swapChainExtent;
    VkCommandPool commandPool;
    std::vector<VkCommandBuffer> commandBuffers;

  private:
    std::function<void(GLFWwindow *, int, int, int)> onMouseButton;
    std::function<void(GLFWwindow *, double, double)> onCursorMove;

  protected:
    virtual void init() = 0;
    virtual void drawFrame(VkCommandBuffer commandBuffer, VkFramebuffer frameBuffer) = 0;
    virtual void cleanup() = 0;

    VkInstance instance;
    virtual void handleEvents() {
        // override this to call things like glfwGetKey and glfwGetMouseButton
    }
    virtual void keyCallback(int key, int /*scancode*/, int action, int /*mods*/);

  private:
    bool enableValidationLayers = true;
    GLFWwindow *window;

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

    static void framebufferResizeCallback(GLFWwindow *window, int /*width*/, int /*height*/);

    void initWindow();
    void initVulkan();
    void cleanupSwapChain();
    void cleanupVulkan();
    void recreateSwapChain();
    void createInstance();
    void populateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT &createInfo);
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
    void copyBufferToImage(VkBuffer buffer, VkImage image, uint32_t width, uint32_t height);
    VkCommandBuffer beginSingleTimeCommands();
    void endSingleTimeCommands(VkCommandBuffer commandBuffer);
    void createSyncObjects();
    void render();
    VkSurfaceFormatKHR chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR> &availableFormats);
    VkPresentModeKHR chooseSwapPresentMode(const std::vector<VkPresentModeKHR> &availablePresentModes);
    VkExtent2D chooseSwapExtent(const VkSurfaceCapabilitiesKHR &capabilities);
    std::vector<const char *> getRequiredExtensions();
    bool checkValidationLayerSupport();
    static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity, VkDebugUtilsMessageTypeFlagsEXT messageType,
                                                        const VkDebugUtilsMessengerCallbackDataEXT *pCallbackData, void * /*pUserData*/);
    static VkResult CreateDebugUtilsMessengerEXT(VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT *pCreateInfo,
                                                 const VkAllocationCallbacks *pAllocator, VkDebugUtilsMessengerEXT *pDebugMessenger);
    static void DestroyDebugUtilsMessengerEXT(VkInstance instance, VkDebugUtilsMessengerEXT debugMessenger, const VkAllocationCallbacks *pAllocator);
    static void dispatchKeyCallback(GLFWwindow *window, int key, int scancode, int action, int mods);
};

#endif // VULK_INCLUDE_H