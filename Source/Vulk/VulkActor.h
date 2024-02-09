#pragma once

#include "Vulk.h"
#include "VulkMesh.h"
#include "VulkModel.h"
#include "VulkPipelineBuilder.h"
#include "VulkUniformBuffer.h"
#include "VulkDescriptorSetBuilder.h"

struct VulkSceneUBOs;

class VulkActor
{
    Vulk &vk;

public:
    std::string name;
    std::shared_ptr<VulkModel> model;
    glm::mat4 xform = glm::mat4(1.0f);
    std::shared_ptr<VulkPipeline> pipeline;
    std::shared_ptr<VulkFrameUBOs<glm::mat4>> xformUBOs;
    std::shared_ptr<VulkDescriptorSetInfo> dsInfo;
    VulkActor(Vulk &vk, std::shared_ptr<VulkModel> model, glm::mat4 xform, std::shared_ptr<VulkPipeline> pipeline, VulkDescriptorSetBuilder &builder);
};