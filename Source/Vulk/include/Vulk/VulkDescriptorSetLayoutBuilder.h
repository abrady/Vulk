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
    VulkDescriptorSetLayoutBuilder& addInputAttachment(VkShaderStageFlags stageFlags, auto bindingIn)
        requires InputAtmtBinding<decltype(bindingIn)>
    {
        uint32_t binding = (uint32_t)bindingIn;
        if (!layoutBindingsMap.contains(binding)) {
            VkDescriptorSetLayoutBinding layoutBinding{};
            layoutBinding.binding         = binding;
            layoutBinding.descriptorType  = VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT;
            layoutBinding.descriptorCount = 1;
            layoutBinding.stageFlags      = stageFlags;
            layoutBindingsMap[binding]    = layoutBinding;
        } else {
            layoutBindingsMap[binding].stageFlags |= stageFlags;
        }
        return *this;
    }
    // and finally, build the layout
    std::shared_ptr<VulkDescriptorSetLayout> build();

   private:
    // can contain vulk::cpp2::VulkShaderUBOBinding, vulk::cpp2::VulkShaderTextureBinding, vulk::cpp2::VulkShaderSSBOBinding
    std::unordered_map<uint32_t, VkDescriptorSetLayoutBinding> layoutBindingsMap;
};
