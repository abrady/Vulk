#pragma once

#include <vulkan/vulkan.h>
#include <memory>
#include <vector>
#include "VulkDescriptorSet.h"
#include "VulkImageView.h"
#include "VulkSampler.h"
#include "VulkStorageBuffer.h"

template <typename T>
class VulkStorageBuffer;

// accumulate and batch-update descriptor sets
// honestly I'm not quite sure what the value of this is
// vs. just calling vkUpdateDescriptorSets directly
class VulkDescriptorSetUpdater {
   private:
    // why did I allocate these? I don't know...
    std::vector<std::unique_ptr<const VkDescriptorBufferInfo>> bufferInfos;
    std::vector<std::unique_ptr<const VkDescriptorImageInfo>> imageInfos;
    std::vector<VkWriteDescriptorSet> descriptorWrites;
    std::shared_ptr<VulkDescriptorSet> descriptorSet;

   public:
    VulkDescriptorSetUpdater(std::shared_ptr<VulkDescriptorSet> descriptorSet) : descriptorSet(descriptorSet) {}

    VulkDescriptorSetUpdater& addUniformBuffer(VkBuffer buf, VkDeviceSize range, vulk::cpp2::VulkShaderUBOBinding binding);
    VulkDescriptorSetUpdater& addImageSampler(std::shared_ptr<const VulkImageView> textureImageView,
                                              std::shared_ptr<const VulkSampler> textureSampler,
                                              vulk::cpp2::VulkShaderTextureBinding binding);
    VulkDescriptorSetUpdater& addStorageBuffer(VkBuffer buf, VkDeviceSize range, vulk::cpp2::VulkShaderSSBOBinding binding);
    template <typename T>
    VulkDescriptorSetUpdater& addVulkStorageBuffer(VulkStorageBuffer<T>& storageBuffer, uint32_t binding) {
        return addStorageBuffer(storageBuffer.buf, storageBuffer.getSize(), binding);
    }

    // e.g. for GBuffer when you need the gbuf as an input attachment
    VulkDescriptorSetUpdater& addInputAttachment(std::shared_ptr<const VulkImageView> textureImageView, auto bindingIn)
        requires InputAtmtBinding<decltype(bindingIn)>
    {
        uint32_t binding = (uint32_t)bindingIn;

        auto imageInfo         = std::make_unique<VkDescriptorImageInfo>();
        imageInfo->imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        imageInfo->imageView   = textureImageView->imageView;
        imageInfo->sampler     = VK_NULL_HANDLE;

        VkWriteDescriptorSet writeDescriptorSet{};
        writeDescriptorSet.sType           = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        writeDescriptorSet.dstSet          = descriptorSet->descriptorSet;
        writeDescriptorSet.descriptorType  = VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT;
        writeDescriptorSet.dstBinding      = binding;
        writeDescriptorSet.pImageInfo      = imageInfo.get();
        writeDescriptorSet.descriptorCount = 1;

        descriptorSet->textureImageViews.push_back(textureImageView);
        descriptorWrites.push_back(writeDescriptorSet);
        imageInfos.push_back(std::move(imageInfo));
        return *this;
    }

    void update(VkDevice device);
};
