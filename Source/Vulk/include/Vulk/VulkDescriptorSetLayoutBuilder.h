#pragma once

#include "Common/ClassNonCopyableNonMovable.h"
#include "VulkUtil.h"
class Vulk;

class VulkDescriptorSetLayout : public ClassNonCopyableNonMovable {
    Vulk &vk;

  public:
    VkDescriptorSetLayout layout;
    VulkDescriptorSetLayout(Vulk &vk, VkDescriptorSetLayout layout, std::vector<VkDescriptorSetLayoutBinding> bindings,
                            VkDescriptorSetLayoutCreateInfo createInfo);
    ~VulkDescriptorSetLayout();

    // just for debugging
    std::vector<VkDescriptorSetLayoutBinding> const bindings;
    VkDescriptorSetLayoutCreateInfo const createInfo{};
};

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
