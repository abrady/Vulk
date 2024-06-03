#pragma once

#include "ClassNonCopyableNonMovable.h"
#include "VulkDescriptorSetLayout.h"
#include "VulkUtil.h"

class VulkDescriptorSetLayoutBuilder {
    Vulk& vk;

public:
    VulkDescriptorSetLayoutBuilder(Vulk& vk)
        : vk(vk) {}
    VulkDescriptorSetLayoutBuilder& addUniformBuffer(VkShaderStageFlags stageFlags, vulk::VulkShaderUBOBinding::type binding);
    VulkDescriptorSetLayoutBuilder& addImageSampler(VkShaderStageFlags stageFlags, vulk::VulkShaderTextureBinding::type binding);
    VulkDescriptorSetLayoutBuilder& addStorageBuffer(VkShaderStageFlags stageFlags, vulk::VulkShaderSSBOBinding::type binding);

    // and finally, build the layout
    std::shared_ptr<VulkDescriptorSetLayout> build();

private:
    std::unordered_map<uint32_t, VkDescriptorSetLayoutBinding> layoutBindingsMap;
};
