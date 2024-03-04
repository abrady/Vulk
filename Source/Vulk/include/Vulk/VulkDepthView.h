#pragma once
#include <vulkan/vulkan.h>

#include "Common/ClassNonCopyableNonMovable.h"
#include "VulkImageView.h"

class Vulk;
class VulkDepthView : public ClassNonCopyableNonMovable {
  public:
    Vulk &vk;
    std::shared_ptr<VulkTextureView> depthView;

    // isUNORM just means load the depth without changing the format - for example loading a normal map.
    VulkDepthView(Vulk &vkIn) : vk(vkIn) {
        VkImage depthImage;
        VkDeviceMemory depthImageMemory;
        VkImageView depthImageView;

        // we need VK_IMAGE_USAGE_SAMPLED_BIT to be able to sample from the depth image in the shader
        vk.createImage(vk.getWidth(), vk.getHeight(), vk.findDepthFormat(), VK_IMAGE_TILING_OPTIMAL,
                       VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, depthImage,
                       depthImageMemory);
        depthImageView = vk.createImageView(depthImage, vk.findDepthFormat(), VK_IMAGE_ASPECT_DEPTH_BIT);
        depthView = std::make_shared<VulkTextureView>(vk, depthImage, depthImageMemory, depthImageView);
    }
};
