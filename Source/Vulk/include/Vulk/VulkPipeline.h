#pragma once
#include "Vulk.h"
#include "VulkShaderModule.h"
#include <vulkan/vulkan.h>

class VulkPipeline : public ClassNonCopyableNonMovable {
  private:
    Vulk &vk;
    std::vector<std::shared_ptr<VulkShaderModule>> shaderModules;

  public:
    VkPipeline pipeline;
    VkPipelineLayout pipelineLayout;
    std::shared_ptr<VulkDescriptorSetLayout> descriptorSetLayout;

    VulkPipeline(Vulk &vk, VkPipeline pipeline, VkPipelineLayout pipelineLayout, std::shared_ptr<VulkDescriptorSetLayout> descriptorSetLayout,
                 std::vector<std::shared_ptr<VulkShaderModule>> shaderModules)
        : vk(vk), shaderModules(shaderModules), pipeline(pipeline), pipelineLayout(pipelineLayout), descriptorSetLayout(descriptorSetLayout) {
    }
    ~VulkPipeline() {
        vkDestroyPipeline(vk.device, pipeline, nullptr);
        vkDestroyPipelineLayout(vk.device, pipelineLayout, nullptr);
    }
};
