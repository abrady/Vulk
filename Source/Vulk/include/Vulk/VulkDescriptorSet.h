#pragma once

#include <memory>
#include <vector>
#include "ClassNonCopyableNonMovable.h"
#include "Vulk.h"
#include "VulkSampler.h"

class VulkImageView;

class VulkDescriptorSet : public ClassNonCopyableNonMovable {
    friend class VulkDescriptorSetUpdater;
    std::vector<std::shared_ptr<const VulkImageView>> textureImageViews;
    std::vector<std::shared_ptr<const VulkSampler>> textureSamplers;

   public:
    VkDescriptorSet descriptorSet;
    VulkDescriptorSet(Vulk& vk, VkDescriptorSetLayout descriptorSetLayout, VkDescriptorPool descriptorPool) {
        descriptorSet = vk.createDescriptorSet(descriptorSetLayout, descriptorPool);
    };
};