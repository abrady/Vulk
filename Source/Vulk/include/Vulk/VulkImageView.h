#pragma once
#include <vulkan/vulkan.h>

#include "ClassNonCopyableNonMovable.h"

class Vulk;
class VulkImageView : public ClassNonCopyableNonMovable {
public:
    Vulk& vk;
    VkImage image;
    VkDeviceMemory imageMemory;
    VkImageView imageView;

    // isUNORM just means load the texture without changing the format - for example loading a normal map.
    VulkImageView(Vulk& vkIn, std::filesystem::path const& texturePath, bool isUNORM);
    VulkImageView(Vulk& vkIn, char const* texturePath, bool isUNORM);
    VulkImageView(Vulk& vkIn, std::string const& texturePath, bool isUNORM)
        : VulkImageView(vkIn, texturePath.c_str(), isUNORM) {}
    VulkImageView(Vulk& vkIn, VkImage depthImage, VkDeviceMemory depthImageMemory, VkImageView depthImageView)
        : vk(vkIn)
        , image(depthImage)
        , imageMemory(depthImageMemory)
        , imageView(depthImageView) {}
    ~VulkImageView();

    VulkImageView(Vulk& vkIn)
        : vk(vkIn) {}
    static std::shared_ptr<VulkImageView> createCubemapView(Vulk& vk, std::array<std::string, 6> const& cubemapImgs);

private:
    void loadTextureView(char const* texturePath, bool isUNORM);
};
