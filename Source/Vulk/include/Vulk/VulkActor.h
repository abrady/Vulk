#pragma once

#include "Vulk.h"
#include "VulkMesh.h"
#include "VulkModel.h"
#include "VulkPipeline.h"
#include "VulkUniformBuffer.h"

struct VulkSceneUBOs;
class VulkDescriptorSetInfo;

class VulkActor {
  public:
    std::string name;
    std::shared_ptr<VulkModel> model;
    std::shared_ptr<VulkFrameUBOs<glm::mat4>> xformUBOs;
    std::shared_ptr<VulkDescriptorSetInfo> dsInfo;
    std::shared_ptr<VulkPipeline> pipeline;
    VulkActor(Vulk &, std::shared_ptr<VulkModel> model, std::shared_ptr<VulkFrameUBOs<glm::mat4>> xformUBOs, std::shared_ptr<VulkDescriptorSetInfo> dsInfo,
              std::shared_ptr<VulkPipeline> pipeline)
        : model(model), xformUBOs(xformUBOs), dsInfo(dsInfo), pipeline(pipeline) {
    }
};