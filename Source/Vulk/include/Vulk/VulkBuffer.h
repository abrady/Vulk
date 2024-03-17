#pragma once

#include "Common/ClassNonCopyableNonMovable.h"
#include "Vulk.h"
#include "Vulk/VulkLogger.h"

struct VulkBuffer : public ClassNonCopyableNonMovable {
    Vulk &vk;
    VkBuffer buf;
    VkDeviceMemory bufMem;

    VulkBuffer(Vulk &vk, VkBuffer buf, VkDeviceMemory bufMem) : vk(vk), buf(buf), bufMem(bufMem) {
        TRACE("VulkBuffer: constructed with buffer {:p} and memory {:p}", (void *)buf, (void *)bufMem);
    }

    ~VulkBuffer() {
        TRACE("VulkBuffer: destructed buffer {:p} and memory {:p}", (void *)buf, (void *)bufMem);
        vkDestroyBuffer(vk.device, buf, nullptr);
        vkFreeMemory(vk.device, bufMem, nullptr);
    }
};
