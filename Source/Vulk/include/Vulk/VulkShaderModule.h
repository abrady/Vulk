#pragma once

#include "Vulk.h"

struct VulkShaderModule {
    Vulk& vk;
    VkShaderModule shaderModule;
    VulkShaderModule(Vulk& vk, VkShaderModule shaderModule) : vk(vk), shaderModule(shaderModule) {}
    ~VulkShaderModule() {
        vkDestroyShaderModule(vk.device, shaderModule, nullptr);
    }
};
