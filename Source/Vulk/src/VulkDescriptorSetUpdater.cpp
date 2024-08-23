#include "Vulk/VulkDescriptorSetUpdater.h"
#include "Vulk/VulkImageView.h"

VulkDescriptorSetUpdater&
VulkDescriptorSetUpdater::addUniformBuffer(VkBuffer buf, VkDeviceSize range, vulk::cpp2::VulkShaderUBOBinding bindingIn) {
    uint32_t binding          = (uint32_t)bindingIn;
    auto uniformBufferInfo    = std::make_unique<VkDescriptorBufferInfo>();
    uniformBufferInfo->buffer = buf;
    uniformBufferInfo->offset = 0;
    uniformBufferInfo->range  = range;

    VkWriteDescriptorSet writeDescriptorSet{};
    writeDescriptorSet.sType           = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    writeDescriptorSet.dstSet          = descriptorSet->descriptorSet;
    writeDescriptorSet.dstBinding      = binding;
    writeDescriptorSet.dstArrayElement = 0;
    writeDescriptorSet.descriptorType  = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    writeDescriptorSet.descriptorCount = 1;
    writeDescriptorSet.pBufferInfo     = uniformBufferInfo.get();
    descriptorWrites.push_back(writeDescriptorSet);
    bufferInfos.push_back(std::move(uniformBufferInfo));
    return *this;
}

VulkDescriptorSetUpdater& VulkDescriptorSetUpdater::addImageSampler(
    std::shared_ptr<VulkImageView> textureImageView,
    std::shared_ptr<VulkSampler> textureSampler,
    vulk::cpp2::VulkShaderTextureBinding bindingIn
) {
    uint32_t binding = (uint32_t)bindingIn;

    auto imageInfo         = std::make_unique<VkDescriptorImageInfo>();
    imageInfo->imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    imageInfo->imageView   = textureImageView->imageView;
    imageInfo->sampler     = textureSampler->get();

    VkWriteDescriptorSet writeDescriptorSet{};
    writeDescriptorSet.sType           = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    writeDescriptorSet.dstSet          = descriptorSet->descriptorSet;
    writeDescriptorSet.dstBinding      = binding;
    writeDescriptorSet.dstArrayElement = 0;
    writeDescriptorSet.descriptorType  = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    writeDescriptorSet.descriptorCount = 1;
    writeDescriptorSet.pImageInfo      = imageInfo.get();

    descriptorSet->textureImageViews.push_back(textureImageView);
    descriptorSet->textureSamplers.push_back(textureSampler);
    descriptorWrites.push_back(writeDescriptorSet);
    imageInfos.push_back(std::move(imageInfo));
    return *this;
}

VulkDescriptorSetUpdater&
VulkDescriptorSetUpdater::addStorageBuffer(VkBuffer buf, VkDeviceSize range, vulk::cpp2::VulkShaderSSBOBinding bindingIn) {
    uint32_t binding    = (uint32_t)bindingIn;
    auto storageInfo    = std::make_unique<VkDescriptorBufferInfo>();
    storageInfo->buffer = buf;
    storageInfo->offset = 0;
    storageInfo->range  = range;

    VkWriteDescriptorSet writeDescriptorSet{};
    writeDescriptorSet.sType           = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    writeDescriptorSet.dstSet          = descriptorSet->descriptorSet;
    writeDescriptorSet.dstBinding      = binding;
    writeDescriptorSet.dstArrayElement = 0;
    writeDescriptorSet.descriptorType  = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    writeDescriptorSet.descriptorCount = 1;
    writeDescriptorSet.pBufferInfo     = storageInfo.get();

    descriptorWrites.push_back(writeDescriptorSet);
    bufferInfos.push_back(std::move(storageInfo));
    return *this;
}

void VulkDescriptorSetUpdater::update(VkDevice device) {
    vkUpdateDescriptorSets(device, static_cast<uint32_t>(descriptorWrites.size()), descriptorWrites.data(), 0, nullptr);
}