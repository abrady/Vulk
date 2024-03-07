#pragma once

#include "Vulk.h"

class VulkSampler {
    Vulk &vk;
    VkSampler sampler;

  public:
    VulkSampler(Vulk &vk, VkSampler sampler) : vk(vk), sampler(sampler) {
    }

    ~VulkSampler() {
        vkDestroySampler(vk.device, sampler, nullptr);
    }

    VkSampler get() const {
        return sampler;
    }

    static std::shared_ptr<VulkSampler> createShadowSampler(Vulk &vk) {
        VkSamplerCreateInfo samplerInfo{};
        samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
        samplerInfo.magFilter = VK_FILTER_LINEAR;
        samplerInfo.minFilter = VK_FILTER_LINEAR;
        samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER; // this setting + borderColor sets the shadow map to 1.0 outside the light frustum
        samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER; // this setting + borderColor sets the shadow map to 1.0 outside the light frustum
        samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER; // this setting + borderColor sets the shadow map to 1.0 outside the light frustum
        samplerInfo.borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE;
        samplerInfo.anisotropyEnable = VK_FALSE;
        samplerInfo.maxAnisotropy = 1.0f;
        samplerInfo.unnormalizedCoordinates = VK_FALSE;
        samplerInfo.compareEnable = VK_TRUE;
        samplerInfo.compareOp = VK_COMPARE_OP_LESS_OR_EQUAL;
        samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
        samplerInfo.mipLodBias = 0.0f;
        samplerInfo.minLod = 0.0f;
        samplerInfo.maxLod = 0.0f;

        VkSampler sampler;
        VK_CALL(vkCreateSampler(vk.device, &samplerInfo, nullptr, &sampler));
        return std::make_shared<VulkSampler>(vk, sampler);
    }

    static std::shared_ptr<VulkSampler> createImageSampler(Vulk &vk) {
        return std::make_shared<VulkSampler>(vk, vk.createTextureSampler());
    }
};