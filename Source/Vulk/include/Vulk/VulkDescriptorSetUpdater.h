#pragma once

#include "VulkDescriptorSet.h"
#include "VulkImageView.h"
#include "VulkSampler.h"
#include "VulkStorageBuffer.h"
#include <memory>
#include <vector>
#include <vulkan/vulkan.h>

template <typename T>
class VulkStorageBuffer;

class VulkDescriptorSetUpdater {
private:
    std::vector<std::unique_ptr<VkDescriptorBufferInfo>> bufferInfos;
    std::vector<std::unique_ptr<VkDescriptorImageInfo>> imageInfos;
    std::vector<VkWriteDescriptorSet> descriptorWrites;
    std::shared_ptr<VulkDescriptorSet> descriptorSet;

public:
    VulkDescriptorSetUpdater(std::shared_ptr<VulkDescriptorSet> descriptorSet)
        : descriptorSet(descriptorSet) {}

    VulkDescriptorSetUpdater& addUniformBuffer(VkBuffer buf, VkDeviceSize range, vulk::cpp2::VulkShaderUBOBinding binding);
    VulkDescriptorSetUpdater& addImageSampler(std::shared_ptr<VulkImageView> textureImageView, std::shared_ptr<VulkSampler> textureSampler,
                                              vulk::cpp2::VulkShaderTextureBinding binding);
    VulkDescriptorSetUpdater& addStorageBuffer(VkBuffer buf, VkDeviceSize range, vulk::cpp2::VulkShaderSSBOBinding binding);
    template <typename T>
    VulkDescriptorSetUpdater& addVulkStorageBuffer(VulkStorageBuffer<T>& storageBuffer, uint32_t binding) {
        return addStorageBuffer(storageBuffer.buf, storageBuffer.getSize(), binding);
    }
    void update(VkDevice device);
};
