#pragma once

#ifdef _MSC_VER           // Check if we're using MSVC
#pragma warning(push, 0)  // assume these headers know what they're doing
#endif

#include "VulkConstants.h"

#include <glm/gtx/quaternion.hpp>
#include <glm/gtx/string_cast.hpp>

#include <stb_image.h>

#ifdef _MSC_VER
#pragma warning(pop)  // Restore original warning settings from before 'push'
#endif

#include <algorithm>
#include <array>
#include <cassert>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <iostream>
#include <limits>
#include <memory>
#include <optional>
#include <set>
#include <stdexcept>
#include <unordered_map>
#include <vector>
// #include <vulkan/vk_enum_string_helper.h>
#include <vulkan/vulkan.h>

#include "VulkException.h"
#include "VulkLogger.h"
// #include "VulkShaderEnums_types.h"
#pragma warning(push)
#pragma warning(disable : 4702)  // unreachable code
#pragma warning(disable : 4267)  // size_t to uint32_t conversion

#include "gen-cpp2/VulkResourceMetadata_data.h"
#include "gen-cpp2/VulkResourceMetadata_types.h"
#include "gen-cpp2/VulkResourceMetadata_types.tcc"
#include "gen-cpp2/VulkShaderEnums_data.h"
#include "gen-cpp2/VulkShaderEnums_types.h"
#include "gen-cpp2/VulkShaderEnums_types.tcc"
#pragma warning(pop)

#define VK_CALL(func)                                                                                       \
    do {                                                                                                    \
        VkResult vkcall_macro_result = (func);                                                              \
        if (vkcall_macro_result != VK_SUCCESS) {                                                            \
            std::cerr << "Vulkan error: " << (vkcall_macro_result) << " at " << __FILE__ << ":" << __LINE__ \
                      << std::endl;                                                                         \
            VULK_THROW("Vulkan error: {}", std::to_string(vkcall_macro_result));                            \
        }                                                                                                   \
    } while (0)

#define ASSERT_KEY_NOT_SET(findable_container, key) assert((findable_container).find(key) == (findable_container).end())
#define ASSERT_KEY_SET(findable_container, key) assert((findable_container).find(key) != (findable_container).end())

#define VULK_TEXTURE_DIR "Assets/Textures/"
#define VULK_SHADERS_DIR "Source/Shaders/"

// keep in sync with Source\Shaders\Common\common.glsl
struct VulkMaterialConstants {
    glm::vec3 Ka;  // Ambient color
    float Ns;      // Specular exponent (shininess)
    glm::vec3 Kd;  // Diffuse color
    float Ni;      // Optical density (index of refraction)
    glm::vec3 Ks;  // Specular color
    float d;       // Transparency (dissolve)
};

struct QueueFamilyIndices {
    std::optional<uint32_t> graphicsFamily;
    std::optional<uint32_t> presentFamily;

    bool isComplete() { return graphicsFamily.has_value() && presentFamily.has_value(); }
};

struct SwapChainSupportDetails {
    VkSurfaceCapabilitiesKHR capabilities;
    std::vector<VkSurfaceFormatKHR> formats;
    std::vector<VkPresentModeKHR> presentModes;
};

class VulkMesh;

SwapChainSupportDetails querySwapChainSupport(VkPhysicalDevice device, VkSurfaceKHR surface);
QueueFamilyIndices findQueueFamilies(VkPhysicalDevice device, VkSurfaceKHR surface);
std::vector<char> readFileIntoMem(const std::string& filename);

class VulkPauseableTimer {
   public:
    VulkPauseableTimer() : isRunning(false), elapsedTime(0.0f) {}

    void start() {
        if (!isRunning) {
            startTime = std::chrono::high_resolution_clock::now();
            isRunning = true;
        }
    }

    void pause() {
        if (isRunning) {
            auto currentTime = std::chrono::high_resolution_clock::now();
            elapsedTime += std::chrono::duration<float>(currentTime - startTime).count();
            isRunning = false;
        }
    }

    void resume() { start(); }

    void toggle() {
        if (isRunning) {
            pause();
        } else {
            resume();
        }
    }

    void reset() {
        isRunning = false;
        elapsedTime = 0.0f;
    }

    float getElapsedTime() {
        if (isRunning) {
            auto currentTime = std::chrono::high_resolution_clock::now();
            auto currentElapsedTime = elapsedTime + std::chrono::duration<float>(currentTime - startTime).count();
            return currentElapsedTime;
        } else {
            return elapsedTime;
        }
    }

   private:
    bool isRunning;
    float elapsedTime;  // In seconds
    std::chrono::time_point<std::chrono::high_resolution_clock> startTime;
};