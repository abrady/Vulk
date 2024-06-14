#include <filesystem>

#include "Vulk/Vulk.h" // must be included before dds.hpp
#include "Vulk/VulkImageView.h"

#include <dds.hpp>

void VulkImageView::loadTextureView(char const* texturePath, bool isUNORM) {
    VkFormat format;
    vk.createTextureImage(texturePath, imageMemory, image, isUNORM, format);
    imageView = vk.createImageView(image, format, VK_IMAGE_ASPECT_COLOR_BIT);
}

VulkImageView::VulkImageView(Vulk& vkIn, std::filesystem::path const& texturePath, bool isUNORM)
    : vk(vkIn) {
    loadTextureView(texturePath.string().c_str(), isUNORM);
}

VulkImageView::VulkImageView(Vulk& vkIn, char const* texturePath, bool isUNORM)
    : vk(vkIn) {
    loadTextureView(texturePath, isUNORM);
}

std::shared_ptr<VulkImageView> VulkImageView::createCubemapView(Vulk& vk, std::filesystem::path const& ddsFile) {
    dds::Image image;
    VULK_ASSERT(dds::readFile(ddsFile.string(), &image) == dds::Success);

    size_t imageSize = image.width * image.height * image.depth * image.dimension;
    VkBuffer stagingBuffer;
    VkDeviceMemory stagingBufferMemory;
    vk.createBuffer(imageSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer, stagingBufferMemory);

    void* data;
    vkMapMemory(vk.device, stagingBufferMemory, 0, imageSize, 0, &data);
    memcpy(data, image.data.data(), static_cast<size_t>(imageSize));
    vkUnmapMemory(vk.device, stagingBufferMemory);

    std::shared_ptr<VulkImageView> cubemap = std::make_shared<VulkImageView>(vk);
    VkImageCreateInfo imageCreateInfo = dds::getVulkanImageCreateInfo(&image);
    VK_CALL(vkCreateImage(vk.device, &imageCreateInfo, nullptr, &cubemap->image));

    VkMemoryRequirements memRequirements;
    vkGetImageMemoryRequirements(vk.device, cubemap->image, &memRequirements);

    VkMemoryAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize = memRequirements.size;
    allocInfo.memoryTypeIndex = vk.findMemoryType(memRequirements.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

    VK_CALL(vkAllocateMemory(vk.device, &allocInfo, nullptr, &cubemap->imageMemory));
    VK_CALL(vkBindImageMemory(vk.device, cubemap->image, cubemap->imageMemory, 0));

    VkCommandBuffer commandBuffer = vk.beginSingleTimeCommands();
    vk.transitionImageLayout(commandBuffer, cubemap->image, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
    vk.endSingleTimeCommands(commandBuffer);

    vk.copyBufferToImage(stagingBuffer, cubemap->image, static_cast<uint32_t>(image.width), static_cast<uint32_t>(image.height));

    commandBuffer = vk.beginSingleTimeCommands();
    vk.transitionImageLayout(commandBuffer, cubemap->image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
    vk.endSingleTimeCommands(commandBuffer);

    vkDestroyBuffer(vk.device, stagingBuffer, nullptr);
    vkFreeMemory(vk.device, stagingBufferMemory, nullptr);

    // VkImageViewCreateInfo imageViewCreateInfo = dds::getVulkanImageViewCreateInfo(&image);
    VkImageViewCreateInfo imageViewInfo = {};
    imageViewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    imageViewInfo.viewType = VK_IMAGE_VIEW_TYPE_CUBE;
    imageViewInfo.format = dds::getVulkanFormat(image.format, image.supportsAlpha);
    imageViewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    imageViewInfo.subresourceRange.baseMipLevel = 0;
    imageViewInfo.subresourceRange.levelCount = image.numMips;
    imageViewInfo.subresourceRange.baseArrayLayer = 0;
    imageViewInfo.subresourceRange.layerCount = image.arraySize;
    VK_CALL(vkCreateImageView(vk.device, &imageViewInfo, nullptr, &cubemap->imageView));

    return cubemap;
}

VulkImageView::~VulkImageView() {
    vkDestroyImageView(vk.device, imageView, nullptr);
    vkDestroyImage(vk.device, image, nullptr);
    vkFreeMemory(vk.device, imageMemory, nullptr);
}
