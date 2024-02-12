#pragma once

#include "VulkUtil.h"
#include "VulkActor.h"
#include "VulkCamera.h"
#include "VulkFrameUBOs.h"
#include "VulkPointLight.h"
#include "VulkUniformBuffer.h"

struct VulkSceneUBOs
{
    struct XformsUBO
    {
        alignas(16) glm::mat4 world;
        alignas(16) glm::mat4 view;
        alignas(16) glm::mat4 proj;
    };

    VulkFrameUBOs<XformsUBO> xforms;
    VulkFrameUBOs<glm::vec3> eyePos;
    VulkUniformBuffer<VulkPointLight> pointLight;
    VulkSceneUBOs(Vulk &vk) : xforms(vk), eyePos(vk), pointLight(vk) {}
};

class VulkScene
{
    Vulk &vk;

public:
    VulkSceneUBOs sceneUBOs;
    VulkCamera camera;
    std::vector<std::shared_ptr<VulkActor>> actors;
    // debug
    std::shared_ptr<VulkUniformBuffer<VulkDebugNormalsUBO>> debugNormalsUBO;
    std::shared_ptr<VulkUniformBuffer<VulkDebugTangentsUBO>> debugTangentsUBO;

    VulkScene(Vulk &vk) : vk(vk), sceneUBOs(vk) {}
};