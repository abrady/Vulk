#pragma once

#include "Common/ClassNonCopyableNonMovable.h"
#include "VulkDescriptorSetLayoutBuilder.h"
#include "VulkPipeline.h"
#include "VulkShaderModule.h"
#include <vulkan/vulkan.h>

class VulkPipelineBuilder {
    Vulk &vk;

    std::vector<std::shared_ptr<VulkShaderModule>> shaderModules;
    std::vector<VkPipelineShaderStageCreateInfo> shaderStages;
    VkVertexInputBindingDescription bindingDescription = {};
    std::vector<VkVertexInputAttributeDescription> attributeDescriptions = {};
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

    VkPrimitiveTopology primitiveTopology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;

    VulkPipelineBuilder &addShaderStage(VkShaderStageFlagBits stage, char const *path);
    VulkPipelineBuilder &addShaderStage(VkShaderStageFlagBits stage, std::shared_ptr<VulkShaderModule> shaderModule);
    VulkPipelineBuilder &addVertexInputField(uint32_t binding, uint32_t location, uint32_t offset, VkFormat format);

  public:
    VulkPipelineBuilder(Vulk &vk);

    VulkPipelineBuilder &addvertShaderStage(std::shared_ptr<VulkShaderModule> shaderModule) {
        return addShaderStage(VK_SHADER_STAGE_VERTEX_BIT, shaderModule);
    }

    VulkPipelineBuilder &addFragmentShaderStage(std::shared_ptr<VulkShaderModule> shaderModule) {
        return addShaderStage(VK_SHADER_STAGE_FRAGMENT_BIT, shaderModule);
    }

    VulkPipelineBuilder &addGeometryShaderStage(std::shared_ptr<VulkShaderModule> shaderModule) {
        return addShaderStage(VK_SHADER_STAGE_GEOMETRY_BIT, shaderModule);
    }

    VulkPipelineBuilder &setPrimitiveTopology(VkPrimitiveTopology topology);
    VulkPipelineBuilder &setLineWidth(float lineWidth);
    VulkPipelineBuilder &setCullMode(VkCullModeFlags cullMode);
    VulkPipelineBuilder &setDepthTestEnabled(bool enabled);
    VulkPipelineBuilder &setDepthWriteEnabled(bool enabled);
    VulkPipelineBuilder &setDepthCompareOp(VkCompareOp compareOp);
    VulkPipelineBuilder &setStencilTestEnabled(bool enabled);
    VulkPipelineBuilder &setFrontStencilFailOp(VkStencilOp failOp);
    VulkPipelineBuilder &setFrontStencilPassOp(VkStencilOp passOp);
    VulkPipelineBuilder &setFrontStencilDepthFailOp(VkStencilOp depthFailOp);
    VulkPipelineBuilder &setFrontStencilCompareOp(VkCompareOp compareOp);
    VulkPipelineBuilder &setFrontStencilCompareMask(uint32_t compareMask);
    VulkPipelineBuilder &setFrontStencilWriteMask(uint32_t writeMask);
    VulkPipelineBuilder &setFrontStencilReference(uint32_t reference);
    VulkPipelineBuilder &copyFrontStencilToBack();

    // The binding says 'verts are in binding 0', and the stride says 'this is how far apart each vertex is'
    // then the field describe fields within the vertices: pos, normal, etc.
    VulkPipelineBuilder &addVertexInputBindingDescription(uint32_t binding, uint32_t stride, VkVertexInputRate inputRate = VK_VERTEX_INPUT_RATE_VERTEX);
    VulkPipelineBuilder &addVertexInputFieldVec3(uint32_t binding, uint32_t location, uint32_t fieldOffset);
    VulkPipelineBuilder &addVertexInputFieldVec2(uint32_t binding, uint32_t location, uint32_t fieldOffset);

    VulkPipelineBuilder &addVulkVertexInput(uint32_t binding);

    VulkPipelineBuilder &setBlending(bool enabled, VkColorComponentFlags colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
                                                                                          VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT);

    VulkPipelineBuilder &setScissor(VkExtent2D extent);
    VulkPipelineBuilder &setViewport(VkExtent2D extent);

    void build(VkRenderPass renderPass, std::shared_ptr<VulkDescriptorSetLayout> descriptorSetLayout, VkPipelineLayout *pipelineLayout,
               VkPipeline *graphicsPipeline);
    std::shared_ptr<VulkPipeline> build(VkRenderPass renderPass, std::shared_ptr<VulkDescriptorSetLayout> descriptorSetLayout) {
        VkPipelineLayout pipelineLayout;
        VkPipeline graphicsPipeline;
        build(renderPass, descriptorSetLayout, &pipelineLayout, &graphicsPipeline);
        return std::make_shared<VulkPipeline>(vk, graphicsPipeline, pipelineLayout, descriptorSetLayout, shaderModules);
    }
};