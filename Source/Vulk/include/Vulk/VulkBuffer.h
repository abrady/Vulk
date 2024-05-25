#pragma once

#include "ClassNonCopyableNonMovable.h"
#include "Vulk.h"
#include "Vulk/VulkLogger.h"

struct VulkBuffer : public ClassNonCopyableNonMovable {
    Vulk &vk;
    VkBuffer buf;
    VkDeviceMemory bufMem;

    VulkBuffer(Vulk &vk, VkBuffer buf, VkDeviceMemory bufMem) : vk(vk), buf(buf), bufMem(bufMem) {
        VULK_TRACE("VulkBuffer: constructed with buffer {:p} and memory {:p}", (void *)buf, (void *)bufMem);
    }

    ~VulkBuffer() {
        VULK_TRACE("VulkBuffer: destructed buffer {:p} and memory {:p}", (void *)buf, (void *)bufMem);
        vkDestroyBuffer(vk.device, buf, nullptr);
        vkFreeMemory(vk.device, bufMem, nullptr);
    }
};
