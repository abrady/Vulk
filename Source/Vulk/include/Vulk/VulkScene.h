#pragma once

#include "VulkActor.h"
#include "VulkCamera.h"
#include "VulkFrameUBOs.h"
#include "VulkPointLight.h"
#include "VulkUniformBuffer.h"
#include "VulkUtil.h"

class VulkDepthView;

struct VulkSceneUBOs {
    struct XformsUBO {
        alignas(16) glm::mat4 world;
        alignas(16) glm::mat4 view;
        alignas(16) glm::mat4 proj;
    };

    VulkFrameUBOs<XformsUBO> xforms;
    VulkFrameUBOs<glm::vec3> eyePos;
    VulkUniformBuffer<VulkPointLight> pointLight;
    VulkSceneUBOs(Vulk &vk) : xforms(vk), eyePos(vk), pointLight(vk) {
    }
};

class VulkScene {
  public:
    VulkSceneUBOs sceneUBOs;
    VulkCamera camera;
    std::vector<std::shared_ptr<VulkActor>> actors;
    std::shared_ptr<VulkUniformBuffer<VulkLightViewProjUBO>> lightViewProjUBO;
    std::array<std::shared_ptr<VulkDepthView>, MAX_FRAMES_IN_FLIGHT> shadowMapViews;

    // debug
    std::shared_ptr<VulkUniformBuffer<VulkDebugNormalsUBO>> debugNormalsUBO;
    std::shared_ptr<VulkUniformBuffer<VulkDebugTangentsUBO>> debugTangentsUBO;

    VulkScene(Vulk &vk) : sceneUBOs(vk) {
    }
};