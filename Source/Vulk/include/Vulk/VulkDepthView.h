#pragma once
#include <vulkan/vulkan.h>

#include "Common/ClassNonCopyableNonMovable.h"

class Vulk;
class VulkDepthView : public ClassNonCopyableNonMovable {
  public:
    Vulk &vk;
    VkImage depthImage;
    VkDeviceMemory depthImageMemory;
    VkImageView depthImageView;

    // isUNORM just means load the depth without changing the format - for example loading a normal map.
    VulkDepthView(Vulk &vkIn) : vk(vkIn) {
        vk.createImage(vk.getWidth(), vk.getHeight(), vk.findDepthFormat(), VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
                       VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, depthImage, depthImageMemory);
        depthImageView = vk.createImageView(depthImage, vk.findDepthFormat(), VK_IMAGE_ASPECT_DEPTH_BIT);
    }
    ~VulkDepthView() {
        vkDestroyImageView(vk.device, depthImageView, nullptr);
        vkDestroyImage(vk.device, depthImage, nullptr);
        vkFreeMemory(vk.device, depthImageMemory, nullptr);
    }

  private:
    void loaddepthView(char const *depthPath, bool isUNORM);
};
