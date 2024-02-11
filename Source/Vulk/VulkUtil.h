#pragma once

#pragma warning(push, 0) // assume these headers know what they're doing

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/hash.hpp>
#include <glm/gtx/euler_angles.hpp>
#include <stb/stb_image.h>
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
#include <vector>
#include <vulkan/vulkan.h>
#include <vulkan/vk_enum_string_helper.h>

#define VK_CALL(func)                                                                                                                                            \
    do                                                                                                                                                           \
    {                                                                                                                                                            \
        VkResult vkcall_macro_result = (func);                                                                                                                   \
        if (vkcall_macro_result != VK_SUCCESS)                                                                                                                   \
        {                                                                                                                                                        \
            std::cerr << "Vulkan error: " << string_VkResult(vkcall_macro_result) << " at " << __FILE__ << ":" << __LINE__ << std::endl;                         \
            throw std::runtime_error(std::string("Vulkan error: ") + string_VkResult(vkcall_macro_result) + " at " + __FILE__ + ":" + std::to_string(__LINE__)); \
        }                                                                                                                                                        \
    } while (0)

#define ASSERT_KEY_NOT_SET(findable_container, key) assert((findable_container).find(key) == (findable_container).end())
#define ASSERT_KEY_SET(findable_container, key) assert((findable_container).find(key) != (findable_container).end())

#define VULK_TEXTURE_DIR "Assets/Textures/"
#define VULK_SHADERS_DIR "Source/Shaders/"

// keep in sync with Source\Shaders\Common\common.glsl
// every binding needs to be globally unique across all shaders in a given pipeline
enum VulkShaderBindings
{
    VulkShaderBinding_XformsUBO = 0,
    VulkShaderBinding_TextureSampler = 1,
    VulkShaderBinding_Actors = 2,
    VulkShaderBinding_Lights = 3,
    VulkShaderBinding_Materials = 4,
    VulkShaderBinding_EyePos = 5,
    VulkShaderBinding_TextureSampler2 = 6,
    VulkShaderBinding_TextureSampler3 = 7,
    VulkShaderBinding_WavesXform = 8,
    VulkShaderBinding_NormalSampler = 9,
    VulkShaderBinding_ModelXform = 10,
    VulkShaderBinding_MirrorPlaneUBO = 11,
    VulkShaderBinding_MaterialUBO = 12,
    VulkShaderBinding_MaxBindingID,
};

enum VulkShaderUBOBindings
{
    VulkShaderUBOBinding_Xforms = VulkShaderBinding_XformsUBO,
    VulkShaderUBOBinding_Lights = VulkShaderBinding_Lights,
    VulkShaderUBOBinding_EyePos = VulkShaderBinding_EyePos,
    VulkShaderUBOBinding_WavesXform = VulkShaderBinding_WavesXform,
    VulkShaderUBOBinding_ModelXform = VulkShaderBinding_ModelXform,
    VulkShaderUBOBinding_MirrorPlaneUBO = VulkShaderBinding_MirrorPlaneUBO,
    VulkShaderUBOBinding_MaterialUBO = VulkShaderBinding_MaterialUBO,
    VulkShaderUBOBinding_MaxBindingID = VulkShaderUBOBinding_MaterialUBO,
};

enum VulkShaderSSBOBindings
{
    VulkShaderSSBOBinding_Actors = VulkShaderBinding_Actors,
    VulkShaderSSBOBinding_Materials = VulkShaderBinding_Materials,
    VulkShaderSSBOBinding_MaxBindingID = VulkShaderSSBOBinding_Materials,
};

enum VulkShaderTextureBindings
{
    VulkShaderTextureBinding_TextureSampler = VulkShaderBinding_TextureSampler,
    VulkShaderTextureBinding_TextureSampler2 = VulkShaderBinding_TextureSampler2,
    VulkShaderTextureBinding_TextureSampler3 = VulkShaderBinding_TextureSampler3,
    VulkShaderTextureBinding_NormalSampler = VulkShaderBinding_NormalSampler,
    VulkShaderTextureBinding_MaxBindingID = VulkShaderTextureBinding_NormalSampler,
};

// keep in sync with Source\Shaders\Common\common.glsl
struct VulkMaterialConstants
{
    glm::vec3 Ka; // Ambient color
    float Ns;     // Specular exponent (shininess)
    glm::vec3 Kd; // Diffuse color
    float Ni;     // Optical density (index of refraction)
    glm::vec3 Ks; // Specular color
    float d;      // Transparency (dissolve)
};

struct QueueFamilyIndices
{
    std::optional<uint32_t> graphicsFamily;
    std::optional<uint32_t> presentFamily;

    bool isComplete()
    {
        return graphicsFamily.has_value() && presentFamily.has_value();
    }
};

struct SwapChainSupportDetails
{
    VkSurfaceCapabilitiesKHR capabilities;
    std::vector<VkSurfaceFormatKHR> formats;
    std::vector<VkPresentModeKHR> presentModes;
};

struct Vertex
{
    glm::vec3 pos;
    glm::vec3 normal;
    glm::vec3 tangent;
    glm::vec2 uv;

    enum BindingLocations
    {
        PosBinding = 0,
        NormalBinding = 1,
        TangentBinding = 2,
        TexCoordBinding = 3,
        NumBindingLocations = 4,
    };
};

class VulkMesh;

SwapChainSupportDetails querySwapChainSupport(VkPhysicalDevice device, VkSurfaceKHR surface);
QueueFamilyIndices findQueueFamilies(VkPhysicalDevice device, VkSurfaceKHR surface);
std::vector<char> readFileIntoMem(const std::string &filename);

#include <glm/glm.hpp>
