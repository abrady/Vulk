#pragma once

#include "ClassNonCopyableNonMovable.h"
#include "VulkDescriptorSetLayout.h"
#include "VulkUtil.h"

class VulkDescriptorSetLayoutBuilder {
    Vulk &vk;

  public:
    VulkDescriptorSetLayoutBuilder(Vulk &vk) : vk(vk) {
    }
    VulkDescriptorSetLayoutBuilder &addUniformBuffer(VkShaderStageFlags stageFlags, VulkShaderUBOBinding binding);
    VulkDescriptorSetLayoutBuilder &addImageSampler(VkShaderStageFlags stageFlags, VulkShaderTextureBinding binding);
    VulkDescriptorSetLayoutBuilder &addStorageBuffer(VkShaderStageFlags stageFlags, VulkShaderSSBOBinding binding);

    // and finally, build the layout
    std::shared_ptr<VulkDescriptorSetLayout> build();

  private:
    std::unordered_map<uint32_t, VkDescriptorSetLayoutBinding> layoutBindingsMap;
};
