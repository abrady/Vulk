#pragma once
#include <vulkan/vulkan.h>

#include "ClassNonCopyableNonMovable.h"
#include "Vulk.h"

class VulkFrameBuffer : public ClassNonCopyableNonMovable {
  Vulk& vk;
  VkFramebuffer framebuffer;

 public:
  VulkFrameBuffer(Vulk& vkIn, std::smart_ptr<T> imageView) : vk(vkIn) {
    // Vk
  }

  ~VulkFrameBuffer() { vkDestroyFramebuffer(vk.device, framebuffer, nullptr); }
};
