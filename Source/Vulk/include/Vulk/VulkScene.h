#pragma once

#include "VulkActor.h"
#include "VulkCamera.h"
#include "VulkFrameUBOs.h"
#include "VulkPointLight.h"
#include "VulkUBO.h"
#include "VulkUniformBuffer.h"
#include "VulkUtil.h"

class VulkDepthView;
struct SceneDef;

struct VulkSceneUBOs {
    struct XformsUBO {
        alignas(16) glm::mat4 world;
        alignas(16) glm::mat4 view;
        alignas(16) glm::mat4 proj;
    };

    VulkFrameUBOs<XformsUBO> xforms;
    VulkFrameUBOs<glm::vec3> eyePos;
    VulkUniformBuffer<VulkPointLight> pointLight;
    VulkSceneUBOs(Vulk& vk) : xforms(vk), eyePos(vk), pointLight(vk) {}
};

class VulkScene {
   public:
    VulkSceneUBOs sceneUBOs;
    std::shared_ptr<SceneDef> def;
    VulkCamera camera;

    // std::vector<std::shared_ptr<VulkActor>> actors;
    std::shared_ptr<VulkUniformBuffer<VulkLightViewProjUBO>> lightViewProjUBO;
    std::array<std::shared_ptr<VulkDepthView>, MAX_FRAMES_IN_FLIGHT> shadowMapViews;
    std::shared_ptr<VulkUniformBuffer<VulkGlobalConstantsUBO>> globalConstantsUBO;

    // debug
    std::shared_ptr<VulkUniformBuffer<VulkDebugNormalsUBO>> debugNormalsUBO;
    std::shared_ptr<VulkUniformBuffer<VulkDebugTangentsUBO>> debugTangentsUBO;
    std::shared_ptr<VulkUniformBuffer<VulkPBRDebugUBO>> pbrDebugUBO;

    VulkScene(Vulk& vk, std::shared_ptr<SceneDef> def) : sceneUBOs(vk), def(def) {}
};