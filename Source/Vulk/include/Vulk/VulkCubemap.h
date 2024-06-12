#pragma once

#include <filesystem>
#include <memory>
#include <vulkan/vulkan.h>

#include "ClassNonCopyableNonMovable.h"

class Vulk;
class VulkCubemap final : public ClassNonCopyableNonMovable {
public:
    Vulk& vk;
    VkImage image;
    VkDeviceMemory imageMemory;
    VkImageView imageView;

    static std::shared_ptr<VulkCubemap> createFromDDS(Vulk& vk, std::filesystem::path const& ddsFile);

    VulkCubemap(Vulk& vk)
        : vk(vk) {}
    ~VulkCubemap();
};
