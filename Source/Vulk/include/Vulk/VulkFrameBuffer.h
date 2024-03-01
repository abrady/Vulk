#pragma once
#include <vulkan/vulkan.h>

#include "Common/ClassNonCopyableNonMovable.h"

class Vulk;
template <typename T> class VulkFrameBuffer : public ClassNonCopyableNonMovable {
  public:
    VulkFrameBuffer(Vulk &vkIn, std::smart_ptr<T> imageView) : vk(vkIn) {
    }
    ~VulkFrameBuffer() {
    }

  private:
    void loadTextureView(char const *texturePath, bool isUNORM);
};
