#pragma once
#include <vulkan/vulkan.h>

#include "Common/ClassNonCopyableNonMovable.h"

class Vulk;
class VulkTextureView : public ClassNonCopyableNonMovable {
  public:
    Vulk &vk;
    VkImage image;
    VkDeviceMemory imageMemory;
    VkImageView imageView;

    // isUNORM just means load the texture without changing the format - for example loading a normal map.
    VulkTextureView(Vulk &vkIn, std::filesystem::path const &texturePath, bool isUNORM);
    VulkTextureView(Vulk &vkIn, char const *texturePath, bool isUNORM);
    VulkTextureView(Vulk &vkIn, std::string const &texturePath, bool isUNORM) : VulkTextureView(vkIn, texturePath.c_str(), isUNORM) {
    }
    VulkTextureView(Vulk &vkIn, VkImage depthImage, VkDeviceMemory depthImageMemory, VkImageView depthImageView)
        : vk(vkIn), image(depthImage), imageMemory(depthImageMemory), imageView(depthImageView) {
    }
    ~VulkTextureView();

  private:
    void loadTextureView(char const *texturePath, bool isUNORM);
};
