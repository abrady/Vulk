#pragma once

#include "VulkDescriptorSetLayoutBuilder.h"
#include "VulkPipeline.h"
#include "VulkShaderModule.h"
#include <vulkan/vulkan.h>

class VulkPipelineBuilder {
    Vulk& vk;

    struct VertInput {
        VkVertexInputBindingDescription binding = {};
        VkVertexInputAttributeDescription attribute = {};
    };

    std::vector<std::shared_ptr<VulkShaderModule>> shaderModules;
    std::vector<VkPipelineShaderStageCreateInfo> shaderStages;
    std::unordered_map<vulk::cpp2::VulkShaderLocation, VertInput> vertInputs;
    VkPipelineInputAssemblyStateCreateInfo inputAssembly{};
    VkPipelineViewportStateCreateInfo viewportState{};
    VkPipelineRasterizationStateCreateInfo rasterizer{};
    VkPipelineMultisampleStateCreateInfo multisampling{};
    VkPipelineDepthStencilStateCreateInfo depthStencil{};
    VkPipelineColorBlendAttachmentState colorBlendAttachment{};
    VkPipelineColorBlendStateCreateInfo colorBlending{};
    std::vector<VkDynamicState> dynamicStates;
    VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
    VkRect2D scissor{};
    VkViewport viewport{};
    std::vector<VkPushConstantRange> pushConstantRanges; // currently only one range is supported but this is here for future proofing

    VulkPipelineBuilder& addShaderStage(VkShaderStageFlagBits stage, char const* path);
    VulkPipelineBuilder& addShaderStage(VkShaderStageFlagBits stage, std::shared_ptr<VulkShaderModule> shaderModule);

public:
    VulkPipelineBuilder(Vulk& vk);

    VulkPipelineBuilder& addvertShaderStage(std::shared_ptr<VulkShaderModule> shaderModule) {
        return addShaderStage(VK_SHADER_STAGE_VERTEX_BIT, shaderModule);
    }

    VulkPipelineBuilder& addFragmentShaderStage(std::shared_ptr<VulkShaderModule> shaderModule) {
        return addShaderStage(VK_SHADER_STAGE_FRAGMENT_BIT, shaderModule);
    }

    VulkPipelineBuilder& addGeometryShaderStage(std::shared_ptr<VulkShaderModule> shaderModule) {
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
    VulkPipelineBuilder& setBlending(bool enabled, VkColorComponentFlags colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT |
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

    void build(VkRenderPass renderPass, std::shared_ptr<VulkDescriptorSetLayout> descriptorSetLayout, VkPipelineLayout* pipelineLayout, VkPipeline* graphicsPipeline);
    std::shared_ptr<VulkPipeline> build(VkRenderPass renderPass, std::shared_ptr<VulkDescriptorSetLayout> descriptorSetLayout) {
        VkPipelineLayout pipelineLayout;
        VkPipeline graphicsPipeline;
        build(renderPass, descriptorSetLayout, &pipelineLayout, &graphicsPipeline);
        return std::make_shared<VulkPipeline>(vk, graphicsPipeline, pipelineLayout, descriptorSetLayout, shaderModules);
    }
};