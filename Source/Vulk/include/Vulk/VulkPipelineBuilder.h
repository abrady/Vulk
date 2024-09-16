#pragma once

#include <vulkan/vulkan.h>
#include "VulkDescriptorSetLayoutBuilder.h"
#include "VulkPipeline.h"
#include "VulkShaderModule.h"

struct PipelineDef;

class VulkPipelineBuilder {
    Vulk& vk;
    std::shared_ptr<const PipelineDef> def;

    struct VertInput {
        VkVertexInputBindingDescription binding     = {};
        VkVertexInputAttributeDescription attribute = {};
    };

    std::vector<std::shared_ptr<const VulkShaderModule>> shaderModules;
    std::vector<VkPipelineShaderStageCreateInfo> shaderStages;
    std::unordered_map<vulk::cpp2::VulkShaderLocation, VertInput> vertInputs;
    VkPipelineInputAssemblyStateCreateInfo inputAssembly{};
    VkPipelineViewportStateCreateInfo viewportState{};
    VkPipelineRasterizationStateCreateInfo rasterizer{};
    VkPipelineMultisampleStateCreateInfo multisampling{};
    VkPipelineDepthStencilStateCreateInfo depthStencil{};
    std::vector<VkPipelineColorBlendAttachmentState> colorBlendAttachments;
    VkPipelineColorBlendStateCreateInfo colorBlending{};
    std::vector<VkDynamicState> dynamicStates;
    VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
    VkRect2D scissor{};
    VkViewport viewport{};
    uint32_t subpass = 0;
    // currently only one range is supported but this is here for future proofing
    std::vector<VkPushConstantRange> pushConstantRanges;

    VulkPipelineBuilder& addShaderStage(VkShaderStageFlagBits stage, char const* path);
    VulkPipelineBuilder& addShaderStage(VkShaderStageFlagBits stage, std::shared_ptr<const VulkShaderModule> shaderModule);

   public:
    VulkPipelineBuilder(Vulk& vk, std::shared_ptr<const PipelineDef> def);

    VulkPipelineBuilder& addvertShaderStage(std::shared_ptr<const VulkShaderModule> shaderModule) {
        return addShaderStage(VK_SHADER_STAGE_VERTEX_BIT, shaderModule);
    }

    VulkPipelineBuilder& addFragmentShaderStage(std::shared_ptr<const VulkShaderModule> shaderModule) {
        return addShaderStage(VK_SHADER_STAGE_FRAGMENT_BIT, shaderModule);
    }

    VulkPipelineBuilder& addGeometryShaderStage(std::shared_ptr<const VulkShaderModule> shaderModule) {
        return addShaderStage(VK_SHADER_STAGE_GEOMETRY_BIT, shaderModule);
    }

    VulkPipelineBuilder& setPrimitiveTopology(VkPrimitiveTopology topology);
    VulkPipelineBuilder& setPolygonMode(VkPolygonMode polygonMode);
    VulkPipelineBuilder& setLineWidth(float lineWidth);
    VulkPipelineBuilder& setCullMode(VkCullModeFlags cullMode);
    VulkPipelineBuilder& setDepthTestEnabled(bool enabled);
    VulkPipelineBuilder& setDepthWriteEnabled(bool enabled);
    VulkPipelineBuilder& setDepthCompareOp(VkCompareOp compareOp);
    VulkPipelineBuilder& setStencilTestEnabled(bool enabled);
    VulkPipelineBuilder& setFrontStencilFailOp(VkStencilOp failOp);
    VulkPipelineBuilder& setFrontStencilPassOp(VkStencilOp passOp);
    VulkPipelineBuilder& setFrontStencilDepthFailOp(VkStencilOp depthFailOp);
    VulkPipelineBuilder& setFrontStencilCompareOp(VkCompareOp compareOp);
    VulkPipelineBuilder& setFrontStencilCompareMask(uint32_t compareMask);
    VulkPipelineBuilder& setFrontStencilWriteMask(uint32_t writeMask);
    VulkPipelineBuilder& setFrontStencilReference(uint32_t reference);
    VulkPipelineBuilder& copyFrontStencilToBack();

    VulkPipelineBuilder& addVertexInput(vulk::cpp2::VulkShaderLocation input);
    VulkPipelineBuilder& addColorBlendAttachment(bool blendingEnabled,
                                                 VkColorComponentFlags colorWriteMask = VK_COLOR_COMPONENT_R_BIT |
                                                                                        VK_COLOR_COMPONENT_G_BIT |
                                                                                        VK_COLOR_COMPONENT_B_BIT |
                                                                                        VK_COLOR_COMPONENT_A_BIT);

    VulkPipelineBuilder& setScissor(VkExtent2D extent);
    VulkPipelineBuilder& setViewport(VkExtent2D extent);

    template <typename T>
    VulkPipelineBuilder& addFragPushConstantRange() {
        return addPushConstantRange(VK_SHADER_STAGE_FRAGMENT_BIT, sizeof(T));
    }
    template <typename T>
    VulkPipelineBuilder& addVertPushConstantRange() {
        return addPushConstantRange(VK_SHADER_STAGE_VERTEX_BIT, sizeof(T));
    }
    template <typename T>
    VulkPipelineBuilder& addPushConstantRange(VkShaderStageFlags stageFlags) {
        return addPushConstantRange(stageFlags, sizeof(T));
    }
    VulkPipelineBuilder& addPushConstantRange(VkShaderStageFlags stageFlags, uint32_t size);

    VulkPipelineBuilder& setSubpass(uint32_t subpassIn) {
        this->subpass = subpassIn;
        return *this;
    }

    void build(VkRenderPass renderPass,
               std::shared_ptr<const VulkDescriptorSetLayout> descriptorSetLayout,
               VkPipelineLayout* pipelineLayout,
               VkPipeline* graphicsPipeline);
    std::shared_ptr<const VulkPipeline> build(VkRenderPass renderPass,
                                              std::shared_ptr<const VulkDescriptorSetLayout> descriptorSetLayout);
};