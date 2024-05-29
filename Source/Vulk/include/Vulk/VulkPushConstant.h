#pragma once
#include <vulkan/vulkan.h>

// push constants go into the pipeline layout info.
class VulkPushConstant {
public:
    VulkPushConstant() {
        // Initialize push constant
        pushConstant = {};
        pushConstant.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
        pushConstant.offset = 0;
        pushConstant.size = sizeof(PushConstantData);
    }

    ~VulkPushConstant() {
        // Cleanup code here if necessary
    }

    VkPushConstantRange getPushConstant() {
        return pushConstant;
    }

    void setPushConstantData(PushConstantData data) {
        pushConstantData = data;
    }

    PushConstantData getPushConstantData() {
        return pushConstantData;
    }

private:
    VkPushConstantRange pushConstant;
    struct PushConstantData {
        // Define your push constant data here
    } pushConstantData;
};