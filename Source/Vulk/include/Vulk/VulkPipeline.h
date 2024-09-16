#pragma once
#include <vulkan/vulkan.h>
#include "Vulk.h"
#include "VulkShaderModule.h"

struct PipelineDef;

class VulkPipeline : public ClassNonCopyableNonMovable {
   private:
    Vulk& vk;
    std::vector<std::shared_ptr<const VulkShaderModule>> shaderModules;

   public:
    std::shared_ptr<const PipelineDef> def;
    VkPipeline pipeline;
    VkPipelineLayout pipelineLayout;
    std::shared_ptr<const VulkDescriptorSetLayout> descriptorSetLayout;

    VulkPipeline(Vulk& vk,
                 std::shared_ptr<const PipelineDef> def,
                 VkPipeline pipeline,
                 VkPipelineLayout pipelineLayout,
                 std::shared_ptr<const VulkDescriptorSetLayout> descriptorSetLayout,
                 std::vector<std::shared_ptr<const VulkShaderModule>> shaderModules)
        : vk(vk),
          def(def),
          shaderModules(shaderModules),
          pipeline(pipeline),
          pipelineLayout(pipelineLayout),
          descriptorSetLayout(descriptorSetLayout) {}
    ~VulkPipeline() {
        vkDestroyPipeline(vk.device, pipeline, nullptr);
        vkDestroyPipelineLayout(vk.device, pipelineLayout, nullptr);
    }
};
