#pragma once

#include "Vulk.h"
#include "VulkBuffer.h"

class VulkBufferBuilder {
    Vulk& vk;
    VkDeviceSize size;
    void const* mem;  // optional memory to fill the buffer with
    VkBufferUsageFlags usage;
    VkMemoryPropertyFlags properties;

   public:
    VulkBufferBuilder(Vulk& vk) : vk(vk), size(0), mem(nullptr), usage(0), properties(0) {}

    VulkBufferBuilder& setSize(VkDeviceSize sizeIn) {
        assert(size == 0);
        assert(sizeIn > 0);
        this->size = sizeIn;
        return *this;
    }

    VulkBufferBuilder& setMem(void const* memIn) {
        assert(mem == nullptr);
        this->mem = memIn;
        return *this;
    }

    VulkBufferBuilder& setUsage(VkBufferUsageFlags usageIn) {
        assert(usage == 0);
        this->usage = usageIn;
        return *this;
    }

    VulkBufferBuilder& setProperties(VkMemoryPropertyFlags propertiesIn) {
        assert(properties == 0);
        this->properties = propertiesIn;
        return *this;
    }

    std::shared_ptr<VulkBuffer> build() {
        assert(size != 0);
        assert(usage != 0);
        assert(properties != 0);
        VkBuffer buf;
        VkDeviceMemory bufMem;
        vk.createBuffer(size, usage, properties, buf, bufMem);
        if (mem) {
            vk.copyMemToBuffer(mem, buf, size);
        }
        return std::make_shared<VulkBuffer>(vk, buf, bufMem);
    }
};