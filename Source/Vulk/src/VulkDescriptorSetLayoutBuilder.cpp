#include "Vulk/VulkDescriptorSetLayoutBuilder.h"
#include "Vulk/Vulk.h"
#include "Vulk/VulkUtil.h"

VulkDescriptorSetLayoutBuilder& VulkDescriptorSetLayoutBuilder::addUniformBuffer(VkShaderStageFlags stageFlags, vulk::cpp2::VulkShaderUBOBinding bindingIn) {
    uint32_t binding = (uint32_t)bindingIn;
    if (!layoutBindingsMap.contains(binding)) {
        VkDescriptorSetLayoutBinding layoutBinding{};
        layoutBinding.binding = binding;
        layoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        layoutBinding.descriptorCount = 1;
        layoutBinding.stageFlags = stageFlags;
        layoutBindingsMap[binding] = layoutBinding;
    } else {
        layoutBindingsMap[binding].stageFlags |= stageFlags;
    }
    return *this;
}

VulkDescriptorSetLayoutBuilder& VulkDescriptorSetLayoutBuilder::addImageSampler(VkShaderStageFlags stageFlags, vulk::cpp2::VulkShaderTextureBinding bindingIn) {
    uint32_t binding = (uint32_t)bindingIn;
    if (!layoutBindingsMap.contains(binding)) {
        VkDescriptorSetLayoutBinding layoutBinding{};
        layoutBinding.binding = binding;
        layoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        layoutBinding.descriptorCount = 1;
        layoutBinding.stageFlags = stageFlags;
        layoutBindingsMap[binding] = layoutBinding;
    } else {
        layoutBindingsMap[binding].stageFlags |= stageFlags;
    }
    return *this;
}

VulkDescriptorSetLayoutBuilder& VulkDescriptorSetLayoutBuilder::addStorageBuffer(VkShaderStageFlags stageFlags, vulk::cpp2::VulkShaderSSBOBinding bindingIn) {
    uint32_t binding = (uint32_t)bindingIn;
    if (!layoutBindingsMap.contains(binding)) {
        VkDescriptorSetLayoutBinding layoutBinding{};
        layoutBinding.binding = binding;
        layoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
        layoutBinding.descriptorCount = 1;
        layoutBinding.stageFlags = stageFlags;
        layoutBindingsMap[binding] = layoutBinding;
    } else {
        layoutBindingsMap[binding].stageFlags |= stageFlags;
    }
    return *this;
}

std::shared_ptr<VulkDescriptorSetLayout> VulkDescriptorSetLayoutBuilder::build() {
    std::vector<VkDescriptorSetLayoutBinding> layoutBindings;
    layoutBindings.reserve(layoutBindingsMap.size());
    for (auto& pair : layoutBindingsMap) {
        layoutBindings.push_back(pair.second);
    }
    VkDescriptorSetLayoutCreateInfo layoutCreateInfo{};
    layoutCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    layoutCreateInfo.bindingCount = static_cast<uint32_t>(layoutBindings.size());
    layoutCreateInfo.pBindings = layoutBindings.data();
    VkDescriptorSetLayout descriptorSetLayout;

    VK_CALL(vkCreateDescriptorSetLayout(vk.device, &layoutCreateInfo, nullptr, &descriptorSetLayout));
    return std::make_shared<VulkDescriptorSetLayout>(vk, descriptorSetLayout, std::move(layoutBindings), std::move(layoutCreateInfo));
}
