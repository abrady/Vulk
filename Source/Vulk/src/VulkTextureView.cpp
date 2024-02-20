#include <filesystem>

#include "VulkTextureView.h"
#include "Vulk.h"

void VulkTextureView::loadTextureView(char const *texturePath, bool isUNORM)
{
    VkFormat format;
    vk.createTextureImage(texturePath, textureImageMemory, textureImage, isUNORM, format);
    textureImageView = vk.createImageView(textureImage, format, VK_IMAGE_ASPECT_COLOR_BIT);
}

VulkTextureView::VulkTextureView(Vulk &vkIn, std::filesystem::path const &texturePath, bool isUNORM) : vk(vkIn)
{
    loadTextureView(texturePath.string().c_str(), isUNORM);
}

VulkTextureView::VulkTextureView(Vulk &vkIn, char const *texturePath, bool isUNORM) : vk(vkIn)
{
    loadTextureView(texturePath, isUNORM);
}

VulkTextureView::~VulkTextureView()
{
    vkDestroyImageView(vk.device, textureImageView, nullptr);
    vkDestroyImage(vk.device, textureImage, nullptr);
    vkFreeMemory(vk.device, textureImageMemory, nullptr);
}
