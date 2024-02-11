#include "VulkActor.h"
#include "VulkScene.h"

using namespace std;

VulkActor::VulkActor(Vulk &vk, shared_ptr<VulkModel> model, glm::mat4 xform, shared_ptr<VulkPipeline> pipeline, VulkDescriptorSetBuilder &builder, VkShaderStageFlagBits modelXformBinding) : vk(vk), model(model),
                                                                                                                                                                                              xform(xform), pipeline(pipeline)
{
    xformUBOs = make_shared<VulkFrameUBOs<glm::mat4>>(vk, xform);
    builder.addFrameUBOs(*xformUBOs, modelXformBinding, VulkShaderUBOBinding_ModelXform);
    dsInfo = builder.build();
}