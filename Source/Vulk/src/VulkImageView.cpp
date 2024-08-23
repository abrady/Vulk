#include <filesystem>

#include "Vulk/Vulk.h"
#include "Vulk/VulkImageView.h"

void VulkImageView::loadTextureView(char const* texturePath, bool isUNORM) {
    VkFormat format;
    vk.createTextureImage(texturePath, imageMemory, image, isUNORM, format);
    imageView = vk.createImageView(image, format, VK_IMAGE_ASPECT_COLOR_BIT);
}

VulkImageView::VulkImageView(Vulk& vkIn, std::filesystem::path const& texturePath, bool isUNORM) : vk(vkIn) {
    loadTextureView(texturePath.string().c_str(), isUNORM);
}

VulkImageView::VulkImageView(Vulk& vkIn, char const* texturePath, bool isUNORM) : vk(vkIn) {
    loadTextureView(texturePath, isUNORM);
}

// usual hoop jumping for vulkan:
// 1. load the images into cpu mem
// 2. make a staging buffer and copy the data to that
// 3. allocate an image and bind it to device mem
// 4. copy the data from the staging buffer to the image
// 5. transition the image to a shader readable format
// 6. create the image view for the image
std::shared_ptr<VulkImageView> VulkImageView::createCubemapView(Vulk& vk, std::array<std::string, 6> const& cubemapImgs) {
    // ===========================================
    // 1. Load the images into CPU memory

    uint32_t width, height, channels;
    std::array<stbi_uc*, 6> pixels;
    for (int i = 0; i < 6; ++i) {
        pixels[i] = stbi_load(
            cubemapImgs[i].c_str(),
            (int*)&width,
            (int*)&height,
            (int*)&channels,
            STBI_rgb_alpha
        );  // NOTE: guessing on last param
        VULK_ASSERT(pixels[i], "Failed to load {}", cubemapImgs[i]);
    }

    // ===========================================
    // 2. Make a staging buffer and copy the data to that

    VkDeviceSize imageSize = width * height * 4;  // Assuming 4 bytes per pixel (e.g., RGBA)
    VkBuffer stagingBuffer;
    VkDeviceMemory stagingBufferMemory;
    vk.createBuffer(
        imageSize * 6,
        VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
        stagingBuffer,
        stagingBufferMemory
    );

    void* data;
    vkMapMemory(vk.device, stagingBufferMemory, 0, imageSize * 6, 0, &data);
    for (int i = 0; i < 6; ++i) {
        memcpy(static_cast<char*>(data) + (imageSize * i), pixels[i], static_cast<size_t>(imageSize));
    }
    vkUnmapMemory(vk.device, stagingBufferMemory);

    for (int i = 0; i < 6; ++i) {
        stbi_image_free(pixels[i]);
    }

    // ===========================================
    // 3. Allocate an image and bind it to device memory

    std::shared_ptr<VulkImageView> cubemap = std::make_shared<VulkImageView>(vk);
    VkFormat format                        = VK_FORMAT_R8G8B8A8_SRGB;  // Assuming 4 bytes per pixel (e.g., RGBA)
    VkImageCreateInfo imageInfo{};
    imageInfo.sType         = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    imageInfo.imageType     = VK_IMAGE_TYPE_2D;
    imageInfo.extent.width  = width;
    imageInfo.extent.height = height;
    imageInfo.extent.depth  = 1;
    imageInfo.mipLevels     = 1;
    imageInfo.arrayLayers   = 6;  // Six faces of the cubemap
    imageInfo.format        = format;
    imageInfo.tiling        = VK_IMAGE_TILING_OPTIMAL;
    imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    imageInfo.usage         = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
    imageInfo.sharingMode   = VK_SHARING_MODE_EXCLUSIVE;
    imageInfo.samples       = VK_SAMPLE_COUNT_1_BIT;
    imageInfo.flags         = VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT;  // Flag to create a cubemap
    VK_CALL(vkCreateImage(vk.device, &imageInfo, nullptr, &cubemap->image));

    VkMemoryRequirements memRequirements;
    vkGetImageMemoryRequirements(vk.device, cubemap->image, &memRequirements);

    VkMemoryAllocateInfo allocInfo{};
    allocInfo.sType           = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize  = memRequirements.size;
    allocInfo.memoryTypeIndex = vk.findMemoryType(memRequirements.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

    VK_CALL(vkAllocateMemory(vk.device, &allocInfo, nullptr, &cubemap->imageMemory));
    VK_CALL(vkBindImageMemory(vk.device, cubemap->image, cubemap->imageMemory, 0));

    VkCommandBuffer commandBuffer = vk.beginSingleTimeCommands();
    vk.transitionImageLayout(
        commandBuffer,
        cubemap->image,
        VK_IMAGE_LAYOUT_UNDEFINED,
        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
        1,
        6
    );
    vk.endSingleTimeCommands(commandBuffer);

    // ===========================================
    // 4. Copy the data from the staging buffer to the image

    commandBuffer = vk.beginSingleTimeCommands();
    VkBufferImageCopy bufferCopyRegions[6];
    for (int face = 0; face < 6; ++face) {
        bufferCopyRegions[face].bufferOffset                    = imageSize * face;
        bufferCopyRegions[face].bufferRowLength                 = 0;
        bufferCopyRegions[face].bufferImageHeight               = 0;
        bufferCopyRegions[face].imageSubresource.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
        bufferCopyRegions[face].imageSubresource.mipLevel       = 0;
        bufferCopyRegions[face].imageSubresource.baseArrayLayer = face;
        bufferCopyRegions[face].imageSubresource.layerCount     = 1;
        bufferCopyRegions[face].imageOffset                     = {0, 0, 0};
        bufferCopyRegions[face].imageExtent                     = {width, height, 1};
    }
    vkCmdCopyBufferToImage(
        commandBuffer,
        stagingBuffer,
        cubemap->image,
        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
        6,
        bufferCopyRegions
    );
    vk.endSingleTimeCommands(commandBuffer);

    // ===========================================
    // 5. Transition the image to a shader readable format

    commandBuffer = vk.beginSingleTimeCommands();
    VkImageMemoryBarrier barrier{};
    barrier.sType                           = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    barrier.oldLayout                       = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    barrier.newLayout                       = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    barrier.srcQueueFamilyIndex             = VK_QUEUE_FAMILY_IGNORED;
    barrier.dstQueueFamilyIndex             = VK_QUEUE_FAMILY_IGNORED;
    barrier.image                           = cubemap->image;
    barrier.subresourceRange.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
    barrier.subresourceRange.baseMipLevel   = 0;
    barrier.subresourceRange.levelCount     = 1;
    barrier.subresourceRange.baseArrayLayer = 0;
    barrier.subresourceRange.layerCount     = 6;

    vkCmdPipelineBarrier(
        commandBuffer,
        VK_PIPELINE_STAGE_TRANSFER_BIT,
        VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
        0,
        0,
        nullptr,
        0,
        nullptr,
        1,
        &barrier
    );
    vk.endSingleTimeCommands(commandBuffer);

    vkDestroyBuffer(vk.device, stagingBuffer, nullptr);
    vkFreeMemory(vk.device, stagingBufferMemory, nullptr);

    // ===========================================
    // 6. Create the image view for the image

    VkImageViewCreateInfo imageViewInfo{};
    imageViewInfo.sType                           = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    imageViewInfo.image                           = cubemap->image;
    imageViewInfo.viewType                        = VK_IMAGE_VIEW_TYPE_CUBE;
    imageViewInfo.format                          = format;
    imageViewInfo.subresourceRange.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
    imageViewInfo.subresourceRange.baseMipLevel   = 0;
    imageViewInfo.subresourceRange.levelCount     = 1;
    imageViewInfo.subresourceRange.baseArrayLayer = 0;
    imageViewInfo.subresourceRange.layerCount     = 6;  // Six faces of the cubemap
    VK_CALL(vkCreateImageView(vk.device, &imageViewInfo, nullptr, &cubemap->imageView));

    return cubemap;
}

VulkImageView::~VulkImageView() {
    vkDestroyImageView(vk.device, imageView, nullptr);
    vkDestroyImage(vk.device, image, nullptr);
    vkFreeMemory(vk.device, imageMemory, nullptr);
}
