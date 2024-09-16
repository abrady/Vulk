#pragma once

#include "Vulk.h"
#include "VulkMesh.h"
#include "VulkModel.h"
#include "VulkPipeline.h"
#include "VulkUniformBuffer.h"

struct VulkSceneUBOs;
class VulkDescriptorSetInfo;

// an actor is a model (the set of renderable bits) plus
// * a transform (where it renders)
// * a pipeline (how it renders)
// * a descriptor set (how it binds to the pipeline)
class VulkActor {
   public:
    std::string name;
    std::shared_ptr<const VulkModel> model;
    // std::shared_ptr<const VulkFrameUBOs<glm::mat4>> xformUBOs;
    std::shared_ptr<const VulkDescriptorSetInfo> dsInfo;
    std::shared_ptr<const VulkPipeline> pipeline;
    VulkActor(Vulk&,
              std::shared_ptr<const VulkModel> model,
              // std::shared_ptr<const VulkFrameUBOs<glm::mat4>> xformUBOs,
              std::shared_ptr<const VulkDescriptorSetInfo> dsInfo,
              std::shared_ptr<const VulkPipeline> pipeline)
        : model(model), /*xformUBOs(xformUBOs),*/ dsInfo(dsInfo), pipeline(pipeline) {}
};