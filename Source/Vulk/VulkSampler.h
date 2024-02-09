#pragma once

#include "Vulk.h"

class VulkSampler
{
    Vulk &vk;
    VkSampler sampler;

public:
    VulkSampler(Vulk &vk) : vk(vk)
    {
        sampler = vk.createTextureSampler();
    }

    ~VulkSampler()
    {
        vkDestroySampler(vk.device, sampler, nullptr);
    }

    VkSampler get() const
    {
        return sampler;
    }
};