#include "VulkActor.h"
#include "VulkScene.h"

using namespace std;

VulkActor::VulkActor(Vulk &vk, shared_ptr<VulkModel> model, glm::mat4 xform, shared_ptr<VulkPipeline> pipeline, VulkDescriptorSetBuilder &builder) : vk(vk), model(model),
                                                                                                                                                     xform(xform), pipeline(pipeline)
{
    xformUBOs = make_shared<VulkFrameUBOs<glm::mat4>>(vk, xform);
    builder.addFrameUBOs(*xformUBOs, VK_SHADER_STAGE_VERTEX_BIT, VulkShaderUBOBinding_ModelXform);
    dsInfo = builder.build();
}