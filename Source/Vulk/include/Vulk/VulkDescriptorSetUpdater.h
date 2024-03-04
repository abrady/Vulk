#pragma once

#include "VulkDescriptorSet.h"
#include "VulkImageView.h"
#include "VulkSampler.h"
#include "VulkStorageBuffer.h"
#include <memory>
#include <vector>
#include <vulkan/vulkan.h>

template <typename T> class VulkStorageBuffer;

class VulkDescriptorSetUpdater {
  private:
    std::vector<std::unique_ptr<VkDescriptorBufferInfo>> bufferInfos;
    std::vector<std::unique_ptr<VkDescriptorImageInfo>> imageInfos;
    std::vector<VkWriteDescriptorSet> descriptorWrites;
    std::shared_ptr<VulkDescriptorSet> descriptorSet;

  public:
    VulkDescriptorSetUpdater(std::shared_ptr<VulkDescriptorSet> descriptorSet) : descriptorSet(descriptorSet) {
    }

    VulkDescriptorSetUpdater &addUniformBuffer(VkBuffer buf, VkDeviceSize range, uint32_t binding);
    VulkDescriptorSetUpdater &addImageSampler(std::shared_ptr<VulkTextureView> textureImageView, std::shared_ptr<VulkSampler> textureSampler, uint32_t binding);
    VulkDescriptorSetUpdater &addStorageBuffer(VkBuffer buf, VkDeviceSize range, uint32_t binding);
    template <typename T> VulkDescriptorSetUpdater &addVulkStorageBuffer(VulkStorageBuffer<T> &storageBuffer, uint32_t binding) {
        return addStorageBuffer(storageBuffer.buf, storageBuffer.getSize(), binding);
    }
    void update(VkDevice device);
};
