#pragma once

#include "Common/ClassNonCopyableNonMovable.h"
#include "Vulk.h"
#include "VulkSampler.h"
#include <memory>
#include <vector>

class VulkTextureView;

class VulkDescriptorSet : public ClassNonCopyableNonMovable {
    friend class VulkDescriptorSetUpdater;
    std::vector<std::shared_ptr<VulkTextureView>> textureImageViews;
    std::vector<std::shared_ptr<VulkSampler>> textureSamplers;

  public:
    VkDescriptorSet descriptorSet;
    VulkDescriptorSet(Vulk &vk, VkDescriptorSetLayout descriptorSetLayout, VkDescriptorPool descriptorPool) {
        descriptorSet = vk.createDescriptorSet(descriptorSetLayout, descriptorPool);
    };
};