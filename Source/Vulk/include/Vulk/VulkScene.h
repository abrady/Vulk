#pragma once

#include "VulkActor.h"
#include "VulkCamera.h"
#include "VulkFrameUBOs.h"
#include "VulkPointLight.h"
#include "VulkUBO.h"
#include "VulkUniformBuffer.h"
#include "VulkUtil.h"

class VulkDepthView;
namespace vulk {
class VulkDeferredRenderpass;
}
struct SceneDef;

struct LightsUBO {
    VulkPointLight lights[(int)vulk::cpp2::VulkLights::NumLights];
};

struct VulkSceneUBOs {
    struct XformsUBO {
        alignas(16) glm::mat4 world;
        alignas(16) glm::mat4 view;
        alignas(16) glm::mat4 proj;
    };

    VulkFrameUBOs<XformsUBO> xforms;
    VulkFrameUBOs<glm::vec3> eyePos;
    VulkUniformBuffer<LightsUBO> lightsUBO;
    VulkSceneUBOs(Vulk& vk) : xforms(vk), eyePos(vk), lightsUBO(vk) {}
};

class VulkScene {
   public:
    VulkSceneUBOs sceneUBOs;
    std::shared_ptr<SceneDef> def;
    VulkCamera camera;

    std::shared_ptr<vulk::VulkDeferredRenderpass> deferredRenderpass;

    mutable std::shared_ptr<VulkUniformBuffer<VulkLightViewProjUBO>> lightViewProjUBO;
    mutable std::array<std::shared_ptr<VulkDepthView>, MAX_FRAMES_IN_FLIGHT> shadowMapViews;
    mutable std::shared_ptr<VulkUniformBuffer<VulkGlobalConstantsUBO>> globalConstantsUBO;
    mutable std::shared_ptr<VulkUniformBuffer<glm::mat4>> invViewProjUBO;

    // debug. we don't allocate these until we need them
    mutable std::shared_ptr<VulkUniformBuffer<VulkDebugNormalsUBO>> debugNormalsUBO;
    mutable std::shared_ptr<VulkUniformBuffer<VulkDebugTangentsUBO>> debugTangentsUBO;
    mutable std::shared_ptr<VulkUniformBuffer<VulkPBRDebugUBO>> pbrDebugUBO;

    VulkScene(Vulk& vk, std::shared_ptr<SceneDef> def) : sceneUBOs(vk), def(def) {}
};