#pragma once

#include "VulkUtil.h"
#include "VulkUniformBuffer.h"

template <typename T>
class VulkFrameUBOs
{
    Vulk &vk;

    void init()
    {
        for (uint32_t i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i)
        {
            VkDeviceSize bufferSize = sizeof(T);
            vk.createBuffer(bufferSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, bufs[i], mems[i]);
            vkMapMemory(vk.device, mems[i], 0, bufferSize, 0, (void **)&ptrs[i]);
        }
    }

    std::array<VkDeviceMemory, MAX_FRAMES_IN_FLIGHT> mems;

public:
    std::array<VkBuffer, MAX_FRAMES_IN_FLIGHT> bufs;
    std::array<T *, MAX_FRAMES_IN_FLIGHT> ptrs;

    explicit VulkFrameUBOs(Vulk &vk) : vk(vk)
    {
        init();
    }

    VulkFrameUBOs(Vulk &vk, T const &rhs) : vk(vk)
    {
        init();
        for (auto &ubo : ptrs)
        {
            *ubo = rhs;
        }
    }

    ~VulkFrameUBOs()
    {
        for (uint32_t i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i)
        {
            vkUnmapMemory(vk.device, mems[i]);
            vkDestroyBuffer(vk.device, bufs[i], nullptr);
            vkFreeMemory(vk.device, mems[i], nullptr);
        }
    }
};