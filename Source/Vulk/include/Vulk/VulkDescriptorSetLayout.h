#pragma once

#include "ClassNonCopyableNonMovable.h"
#include "Vulk.h"
#include "VulkUtil.h"
#include <vector>
#include <vulkan/vulkan.h>

class VulkDescriptorSetLayout : public ClassNonCopyableNonMovable {
    Vulk &vk;

  public:
    VkDescriptorSetLayout layout;
    VulkDescriptorSetLayout(Vulk &vk, VkDescriptorSetLayout layout, std::vector<VkDescriptorSetLayoutBinding> bindings,
                            VkDescriptorSetLayoutCreateInfo createInfo)
        : vk(vk), layout(layout), bindings(bindings), createInfo(createInfo) {
    }
    ~VulkDescriptorSetLayout() {
        vkDestroyDescriptorSetLayout(vk.device, layout, nullptr);
    }

    // just for debugging
    std::vector<VkDescriptorSetLayoutBinding> const bindings;
    VkDescriptorSetLayoutCreateInfo const createInfo{};
};
