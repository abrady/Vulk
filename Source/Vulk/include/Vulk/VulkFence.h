#pragma once

#include "ClassNonCopyableNonMovable.h"
#include "Vulk.h"

class VulkFence : public ClassNonCopyableNonMovable {
    Vulk& vk;

public:
    VkFence fence;

    VulkFence(Vulk& vk)
        : vk(vk) {
        VkFenceCreateInfo fenceInfo{};
        fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
        fenceInfo.flags = 0;
        VK_CALL(vkCreateFence(vk.device, &fenceInfo, nullptr, &fence));
    }

    ~VulkFence() {
        vkDestroyFence(vk.device, fence, nullptr);
    }

    void wait() {
        vkWaitForFences(vk.device, 1, &fence, VK_TRUE, UINT64_MAX);
    }

    void reset() {
        vkResetFences(vk.device, 1, &fence);
    }

    VkFence get() {
        return fence;
    }
};