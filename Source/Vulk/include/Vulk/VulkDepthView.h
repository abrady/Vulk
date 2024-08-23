#pragma once
#include <vulkan/vulkan.h>

#include "ClassNonCopyableNonMovable.h"
#include "VulkImageView.h"

class Vulk;
class VulkDepthView : public ClassNonCopyableNonMovable {
   public:
    Vulk& vk;
    std::shared_ptr<VulkImageView> depthView;
    VkExtent2D extent;
    VkFormat depthFormat;

    // isUNORM just means load the depth without changing the format - for example loading a normal map.
    VulkDepthView(Vulk& vkIn, VkExtent2D extentIn, VkFormat depthFormatIn)
        : vk(vkIn), extent(extentIn), depthFormat(depthFormatIn) {
        VkImage depthImage;
        VkDeviceMemory depthImageMemory;
        VkImageView depthImageView;

        vk.createImage(
            extent.width,
            extent.height,
            depthFormat,
            VK_IMAGE_TILING_OPTIMAL,
            VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
            VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
            depthImage,
            depthImageMemory
        );
        depthImageView = vk.createImageView(depthImage, depthFormat, VK_IMAGE_ASPECT_DEPTH_BIT);
        depthView      = std::make_shared<VulkImageView>(vk, depthImage, depthImageMemory, depthImageView);
    }
};
