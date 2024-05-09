#include "Vulk/VulkPipelineBuilder.h"
#include "Vulk/Vulk.h"
#include "Vulk/VulkUtil.h"

VulkPipelineBuilder::VulkPipelineBuilder(Vulk &vk) : vk(vk) {
    rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rasterizer.depthClampEnable = VK_FALSE;
    rasterizer.rasterizerDiscardEnable = VK_FALSE;
    rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
    rasterizer.lineWidth = 1.0f;
    rasterizer.cullMode = VK_CULL_MODE_BACK_BIT; // or VK_CULL_MODE_NONE; for no culling
    rasterizer.frontFace = VK_FRONT_FACE_CLOCKWISE;
    rasterizer.depthBiasEnable = VK_FALSE;

    inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    inputAssembly.primitiveRestartEnable = VK_FALSE;

    viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewportState.viewportCount = 0;
    viewportState.scissorCount = 0;

    multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    multisampling.sampleShadingEnable = VK_FALSE;
    multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

    depthStencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
    depthStencil.depthTestEnable = VK_TRUE;
    depthStencil.depthWriteEnable = VK_TRUE;
    depthStencil.depthCompareOp = VK_COMPARE_OP_LESS;
    depthStencil.depthBoundsTestEnable = VK_FALSE;
    depthStencil.stencilTestEnable = VK_FALSE;

    colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
    colorBlendAttachment.blendEnable = VK_FALSE;

    colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    colorBlending.logicOpEnable = VK_FALSE;
    colorBlending.logicOp = VK_LOGIC_OP_COPY;
    colorBlending.attachmentCount = 1;
    colorBlending.pAttachments = &colorBlendAttachment;
    colorBlending.blendConstants[0] = 0.0f;
    colorBlending.blendConstants[1] = 0.0f;
    colorBlending.blendConstants[2] = 0.0f;
    colorBlending.blendConstants[3] = 0.0f;

    // dynamicStates = {VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR};
}

VulkPipelineBuilder &VulkPipelineBuilder::addShaderStage(VkShaderStageFlagBits stage, char const *path) {
    auto shaderCode = readFileIntoMem(path);
    VkShaderModule shaderModule = vk.createShaderModule(shaderCode);
    addShaderStage(stage, std::make_shared<VulkShaderModule>(vk, shaderModule));
    return *this;
}

VulkPipelineBuilder &VulkPipelineBuilder::addShaderStage(VkShaderStageFlagBits stage, std::shared_ptr<VulkShaderModule> shaderModule) {
    VkPipelineShaderStageCreateInfo shaderStageInfo{};
    shaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    shaderStageInfo.stage = stage;
    shaderStageInfo.module = shaderModule->shaderModule;
    shaderStageInfo.pName = "main"; // entrypoint, by convention

    shaderModules.push_back(shaderModule);
    shaderStages.push_back(shaderStageInfo);
    return *this;
}

VulkPipelineBuilder &VulkPipelineBuilder::setPrimitiveTopology(VkPrimitiveTopology topology) {
    inputAssembly.topology = topology;
    return *this;
}

VulkPipelineBuilder &VulkPipelineBuilder::setLineWidth(float lineWidth) {
    VkPhysicalDeviceProperties deviceProperties;
    vkGetPhysicalDeviceProperties(vk.physicalDevice, &deviceProperties);
    assert(deviceProperties.limits.lineWidthRange[0] <= lineWidth && lineWidth <= deviceProperties.limits.lineWidthRange[1]);
    rasterizer.lineWidth = lineWidth;
    return *this;
}

VulkPipelineBuilder &VulkPipelineBuilder::setCullMode(VkCullModeFlags cullMode) {
    rasterizer.cullMode = cullMode;
    return *this;
}

VulkPipelineBuilder &VulkPipelineBuilder::setDepthTestEnabled(bool enabled) {
    depthStencil.depthTestEnable = enabled;
    return *this;
}
VulkPipelineBuilder &VulkPipelineBuilder::setDepthWriteEnabled(bool enabled) {
    depthStencil.depthWriteEnable = enabled;
    return *this;
}

VulkPipelineBuilder &VulkPipelineBuilder::setDepthCompareOp(VkCompareOp compareOp) {
    depthStencil.depthCompareOp = compareOp;
    return *this;
}

VulkPipelineBuilder &VulkPipelineBuilder::addVertexInput(VulkShaderLocation location) {
    VULK_THROW_IF(vertInputs.find(location) != vertInputs.end(), "Vertex input location already exists");
    VkFormat format;
    uint32_t stride;

    switch (location) {
    case VulkShaderLocation_Pos:
    case VulkShaderLocation_Normal:
    case VulkShaderLocation_Tangent:
        format = VK_FORMAT_R32G32B32_SFLOAT;
        stride = sizeof(glm::vec3);
        break;
    case VulkShaderLocation_TexCoord:
        format = VK_FORMAT_R32G32_SFLOAT;
        stride = sizeof(glm::vec2);
        break;
    default:
        VULK_THROW("Unknown vertex input location");
    };

    VkVertexInputBindingDescription binding = {};
    binding.binding = location;                      // We'll have a single vertex buffer, so we use binding point 0
    binding.stride = stride;                         // Size of a single Vertex object
    binding.inputRate = VK_VERTEX_INPUT_RATE_VERTEX; // Move to the next data entry after each vertex

    VkVertexInputAttributeDescription attribute{};
    attribute.binding = location;
    attribute.location = location;
    attribute.format = format;
    attribute.offset = 0;

    VertInput input = {
        .binding = binding,
        .attribute = attribute,
    };
    vertInputs[location] = input;

    return *this;
}

// enabling means the existing value in the framebuffer will be blended with the new value output from the shader
// while disabling it will just overwrite the existing value
VulkPipelineBuilder &VulkPipelineBuilder::setBlending(bool enabled, VkColorComponentFlags colorWriteMask) {
    colorBlendAttachment.blendEnable = enabled;
    if (enabled) {
        colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
        colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
        colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;
        colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
        colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
        colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;
    }
    colorBlendAttachment.colorWriteMask = colorWriteMask;
    return *this;
}

void VulkPipelineBuilder::build(VkRenderPass renderPass, std::shared_ptr<VulkDescriptorSetLayout> descriptorSetLayout, VkPipelineLayout *pipelineLayout,
                                VkPipeline *graphicsPipeline) {
    assert(viewport.maxDepth > 0.f);

    std::vector<VkVertexInputBindingDescription> bindingDescriptions;
    std::vector<VkVertexInputAttributeDescription> attributeDescriptions;
    for (auto &input : vertInputs) {
        bindingDescriptions.push_back(input.second.binding);
        attributeDescriptions.push_back(input.second.attribute);
    }

    VkPipelineVertexInputStateCreateInfo vertexInputInfo = {};
    vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vertexInputInfo.vertexBindingDescriptionCount = static_cast<uint32_t>(bindingDescriptions.size());
    vertexInputInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(attributeDescriptions.size());
    vertexInputInfo.pVertexBindingDescriptions = bindingDescriptions.data();
    vertexInputInfo.pVertexAttributeDescriptions = attributeDescriptions.data();

    pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipelineLayoutInfo.setLayoutCount = 1;
    pipelineLayoutInfo.pSetLayouts = &descriptorSetLayout->layout;

    VK_CALL(vkCreatePipelineLayout(vk.device, &pipelineLayoutInfo, nullptr, pipelineLayout));

    VkPipelineDynamicStateCreateInfo dynamicState{};
    dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    dynamicState.dynamicStateCount = static_cast<uint32_t>(dynamicStates.size());
    dynamicState.pDynamicStates = dynamicStates.data();

    VkGraphicsPipelineCreateInfo pipelineInfo{};
    pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pipelineInfo.stageCount = static_cast<uint32_t>(shaderStages.size());
    pipelineInfo.pStages = shaderStages.data();
    pipelineInfo.pVertexInputState = &vertexInputInfo;
    pipelineInfo.pInputAssemblyState = &inputAssembly;
    pipelineInfo.pViewportState = &viewportState;
    pipelineInfo.pRasterizationState = &rasterizer;
    pipelineInfo.pMultisampleState = &multisampling;
    pipelineInfo.pDepthStencilState = &depthStencil;
    pipelineInfo.pColorBlendState = &colorBlending;
    pipelineInfo.pDynamicState = &dynamicState;
    pipelineInfo.layout = *pipelineLayout;
    pipelineInfo.renderPass = renderPass;
    pipelineInfo.subpass = 0;
    pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;

    VK_CALL(vkCreateGraphicsPipelines(vk.device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, graphicsPipeline));
}

VulkPipelineBuilder &VulkPipelineBuilder::setStencilTestEnabled(bool enabled) {
    depthStencil.stencilTestEnable = enabled;
    return *this;
}

VulkPipelineBuilder &VulkPipelineBuilder::setFrontStencilFailOp(VkStencilOp failOp) {
    depthStencil.front.failOp = failOp;
    return *this;
}

VulkPipelineBuilder &VulkPipelineBuilder::setFrontStencilPassOp(VkStencilOp passOp) {
    depthStencil.front.passOp = passOp;
    return *this;
}

VulkPipelineBuilder &VulkPipelineBuilder::setFrontStencilDepthFailOp(VkStencilOp depthFailOp) {
    depthStencil.front.depthFailOp = depthFailOp;
    return *this;
}

VulkPipelineBuilder &VulkPipelineBuilder::setFrontStencilCompareOp(VkCompareOp compareOp) {
    depthStencil.front.compareOp = compareOp;
    return *this;
}

VulkPipelineBuilder &VulkPipelineBuilder::setFrontStencilCompareMask(uint32_t compareMask) {
    depthStencil.front.compareMask = compareMask;
    return *this;
}

VulkPipelineBuilder &VulkPipelineBuilder::setFrontStencilWriteMask(uint32_t writeMask) {
    depthStencil.front.writeMask = writeMask;
    return *this;
}

VulkPipelineBuilder &VulkPipelineBuilder::setFrontStencilReference(uint32_t reference) {
    depthStencil.front.reference = reference;
    return *this;
}

VulkPipelineBuilder &VulkPipelineBuilder::copyFrontStencilToBack() {
    depthStencil.back = depthStencil.front;
    return *this;
}

VulkPipelineBuilder &VulkPipelineBuilder::setScissor(VkExtent2D extent) {
    assert(viewportState.pScissors == nullptr);
    scissor.offset = {0, 0};
    scissor.extent = extent;
    viewportState.scissorCount = 1;
    viewportState.pScissors = &scissor;
    return *this;
}

VulkPipelineBuilder &VulkPipelineBuilder::setViewport(VkExtent2D extent) {
    assert(viewportState.pViewports == nullptr);
    viewport.x = 0.0f;
    viewport.y = 0.0f;
    viewport.width = static_cast<float>(extent.width);
    viewport.height = static_cast<float>(extent.height);
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;

    viewportState.viewportCount = 1;
    viewportState.pViewports = &viewport;
    return *this;
}