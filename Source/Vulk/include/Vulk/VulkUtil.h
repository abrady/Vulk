#pragma once

#pragma warning(push, 0) // assume these headers know what they're doing

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#define GLM_FORCE_EXPLICIT_CTOR
#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/euler_angles.hpp>
#include <glm/gtx/hash.hpp>
#include <stb_image.h>
#include <tiny_obj_loader.h>

#pragma warning(pop)

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
#include "VulkShaderEnums_generated.h"

#define VK_CALL(func)                                                                                                                                          \
    do {                                                                                                                                                       \
        VkResult vkcall_macro_result = (func);                                                                                                                 \
        if (vkcall_macro_result != VK_SUCCESS) {                                                                                                               \
            std::cerr << "Vulkan error: " << (vkcall_macro_result) << " at " << __FILE__ << ":" << __LINE__ << std::endl;                                      \
            VULK_THROW(std::string("Vulkan error: ") + std::to_string(vkcall_macro_result) + " at " + __FILE__ + ":" + std::to_string(__LINE__));              \
        }                                                                                                                                                      \
    } while (0)

#define ASSERT_KEY_NOT_SET(findable_container, key) assert((findable_container).find(key) == (findable_container).end())
#define ASSERT_KEY_SET(findable_container, key) assert((findable_container).find(key) != (findable_container).end())

#define VULK_TEXTURE_DIR "Assets/Textures/"
#define VULK_SHADERS_DIR "Source/Shaders/"

// keep in sync with Source\Shaders\Common\common.glsl
struct VulkMaterialConstants {
    glm::vec3 Ka; // Ambient color
    float Ns;     // Specular exponent (shininess)
    glm::vec3 Kd; // Diffuse color
    float Ni;     // Optical density (index of refraction)
    glm::vec3 Ks; // Specular color
    float d;      // Transparency (dissolve)
};

struct QueueFamilyIndices {
    std::optional<uint32_t> graphicsFamily;
    std::optional<uint32_t> presentFamily;

    bool isComplete() {
        return graphicsFamily.has_value() && presentFamily.has_value();
    }
};

struct SwapChainSupportDetails {
    VkSurfaceCapabilitiesKHR capabilities;
    std::vector<VkSurfaceFormatKHR> formats;
    std::vector<VkPresentModeKHR> presentModes;
};

struct Vertex {
    glm::vec3 pos;
    glm::vec3 normal;
    glm::vec3 tangent;
    glm::vec2 uv;

    template <class Archive> void serialize(Archive &archive) {
        archive(CEREAL_NVP(pos), CEREAL_NVP(normal), CEREAL_NVP(tangent), CEREAL_NVP(uv));
    }
};

struct VulkDebugNormalsUBO {
    float length = .1f;    // how long to render the debug normal
    bool useModel = false; // use the model's normals/tangents instead of the shader sampled normals
};

struct VulkDebugTangentsUBO {
    float length = .1f; // how long to render the debug tangent
};

struct VulkLightViewProjUBO {
    glm::mat4 viewProj;
};

class VulkMesh;

SwapChainSupportDetails querySwapChainSupport(VkPhysicalDevice device, VkSurfaceKHR surface);
QueueFamilyIndices findQueueFamilies(VkPhysicalDevice device, VkSurfaceKHR surface);
std::vector<char> readFileIntoMem(const std::string &filename);

class VulkPauseableTimer {
  public:
    VulkPauseableTimer() : isRunning(false), elapsedTime(0.0f) {
    }

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

    void resume() {
        start();
    }

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
    float elapsedTime; // In seconds
    std::chrono::time_point<std::chrono::high_resolution_clock> startTime;
};