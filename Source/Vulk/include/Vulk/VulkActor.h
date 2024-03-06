#pragma once

#include "Vulk.h"
#include "VulkMesh.h"
#include "VulkModel.h"
#include "VulkPipelineBuilder.h"
#include "VulkUniformBuffer.h"

struct VulkSceneUBOs;
class VulkDescriptorSetInfo;

class VulkActor {
    Vulk &vk;

  public:
    std::string name;
    std::shared_ptr<VulkModel> model;
    std::shared_ptr<VulkFrameUBOs<glm::mat4>> xformUBOs;
    std::shared_ptr<VulkDescriptorSetInfo> dsInfo;
    std::shared_ptr<VulkPipeline> pipeline;
    VulkActor(Vulk &vk, std::shared_ptr<VulkModel> model, std::shared_ptr<VulkFrameUBOs<glm::mat4>> xformUBOs, std::shared_ptr<VulkDescriptorSetInfo> dsInfo,
              std::shared_ptr<VulkPipeline> pipeline)
        : vk(vk), model(model), xformUBOs(xformUBOs), dsInfo(dsInfo), pipeline(pipeline) {
    }
};