#include <filesystem>

#include "Vulk/Vulk.h"
#include "Vulk/VulkImageView.h"

void VulkTextureView::loadTextureView(char const *texturePath, bool isUNORM) {
    VkFormat format;
    vk.createTextureImage(texturePath, imageMemory, image, isUNORM, format);
    imageView = vk.createImageView(image, format, VK_IMAGE_ASPECT_COLOR_BIT);
}

VulkTextureView::VulkTextureView(Vulk &vkIn, std::filesystem::path const &texturePath, bool isUNORM) : vk(vkIn) {
    loadTextureView(texturePath.string().c_str(), isUNORM);
}

VulkTextureView::VulkTextureView(Vulk &vkIn, char const *texturePath, bool isUNORM) : vk(vkIn) {
    loadTextureView(texturePath, isUNORM);
}

VulkTextureView::~VulkTextureView() {
    vkDestroyImageView(vk.device, imageView, nullptr);
    vkDestroyImage(vk.device, image, nullptr);
    vkFreeMemory(vk.device, imageMemory, nullptr);
}
