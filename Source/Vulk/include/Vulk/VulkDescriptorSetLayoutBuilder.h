#pragma once

#include "ClassNonCopyableNonMovable.h"
#include "VulkDescriptorSetLayout.h"
#include "VulkUtil.h"

class VulkDescriptorSetLayoutBuilder {
    Vulk& vk;

   public:
    VulkDescriptorSetLayoutBuilder(Vulk& vk) : vk(vk) {}
    VulkDescriptorSetLayoutBuilder& addUniformBuffer(VkShaderStageFlags stageFlags, vulk::cpp2::VulkShaderUBOBinding binding);
    VulkDescriptorSetLayoutBuilder& addImageSampler(VkShaderStageFlags stageFlags, vulk::cpp2::VulkShaderTextureBinding binding);
    VulkDescriptorSetLayoutBuilder& addStorageBuffer(VkShaderStageFlags stageFlags, vulk::cpp2::VulkShaderSSBOBinding binding);

    // and finally, build the layout
    std::shared_ptr<VulkDescriptorSetLayout> build();

   private:
    // can contain vulk::cpp2::VulkShaderUBOBinding, vulk::cpp2::VulkShaderTextureBinding, vulk::cpp2::VulkShaderSSBOBinding
    std::unordered_map<uint32_t, VkDescriptorSetLayoutBinding> layoutBindingsMap;
};
