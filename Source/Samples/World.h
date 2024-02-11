#pragma once

#include "Vulk/Vulk.h"
#include "Vulk/VulkGeo.h"
#include "Vulk/VulkActor.h"
#include "Vulk/VulkCamera.h"
#include "Vulk/VulkPipelineBuilder.h"
#include "Vulk/VulkDescriptorPoolBuilder.h"
#include "Vulk/VulkUniformBuffer.h"
#include "Vulk/VulkStorageBuffer.h"
#include "Vulk/VulkDescriptorSetUpdater.h"
#include "Vulk/VulkTextureView.h"
#include "Vulk/VulkDescriptorSetBuilder.h"
#include "Vulk/VulkBufferBuilder.h"
#include "Vulk/VulkResources.h"
#include "Vulk/VulkResourceMetadata.h"
#include "Vulk/VulkScene.h"

class World
{
public:
    Vulk &vk;
    std::shared_ptr<VulkScene> scene;
    std::shared_ptr<VulkPipeline> debugNormalsPipeline;
    std::vector<std::shared_ptr<VulkActor>> debugNormalsActors;

    float nearClip = 1.f;
    float farClip = 10000.f;

public:
    World(Vulk &vk, std::string sceneName) : vk(vk)
    {
        VulkResources resources(vk);
        resources.loadScene(sceneName);
        scene = resources.scenes[sceneName];
        debugNormalsPipeline = resources.getPipeline("DebugNormals");
        auto debugNormalsPipelineDef = resources.metadata.pipelines.at("DebugNormals");

        SceneDef &sceneDef = *resources.metadata.scenes.at(sceneName);
        for (int i = 0; i < scene->actors.size(); ++i)
        {
            auto actor = scene->actors[i];
            auto actorDef = sceneDef.actors[i];
            std::shared_ptr<VulkActor> debugNormalsActor = resources.createActorFromPipeline(*actorDef, debugNormalsPipelineDef, scene);
            debugNormalsActors.push_back(debugNormalsActor);
        }
    }

    void updateXformsUBO(VulkSceneUBOs::XformsUBO &ubo, VkViewport const &viewport)
    {
        static auto startTime = std::chrono::high_resolution_clock::now();
        auto currentTime = std::chrono::high_resolution_clock::now();
        float time = std::chrono::duration<float, std::chrono::seconds::period>(currentTime - startTime).count();
        time = 0.0f;
        ubo.world = glm::rotate(glm::mat4(1.0f), time * glm::radians(90.0f), glm::vec3(0.0f, 1.0f, 0.0f));
        glm::vec3 fwd = scene->camera.getForwardVec();
        glm::vec3 lookAt = scene->camera.eye + fwd;
        glm::vec3 up = scene->camera.getUpVec();
        ubo.view = glm::lookAt(scene->camera.eye, lookAt, up);
        ubo.proj = glm::perspective(glm::radians(45.0f), viewport.width / (float)viewport.height, nearClip, farClip);
        ubo.proj[1][1] *= -1;
    }

    void render(VkCommandBuffer commandBuffer, uint32_t currentFrame, VkViewport const &viewport, VkRect2D const &scissor)
    {
        updateXformsUBO(*scene->sceneUBOs.xforms.ptrs[currentFrame], viewport);
        *scene->sceneUBOs.eyePos.ptrs[currentFrame] = scene->camera.eye;

        // render the scene?
        for (auto &actor : scene->actors)
        {
            auto model = actor->model;
            vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, actor->pipeline->pipeline);
            vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, actor->pipeline->pipelineLayout, 0, 1, &actor->dsInfo->descriptorSets[currentFrame]->descriptorSet, 0, nullptr);
            vkCmdSetViewport(commandBuffer, 0, 1, &viewport);
            vkCmdSetScissor(commandBuffer, 0, 1, &scissor);

            VkDeviceSize offsets[] = {0};
            vkCmdBindVertexBuffers(commandBuffer, 0, 1, &model->vertBuf.buf, offsets);
            vkCmdBindIndexBuffer(commandBuffer, model->indexBuf.buf, 0, VK_INDEX_TYPE_UINT32);
            vkCmdDrawIndexed(commandBuffer, model->numIndices, 1, 0, 0, 0);
        }

        // render the debug normals
        for (auto &actor : debugNormalsActors)
        {
            auto model = actor->model;
            vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, actor->pipeline->pipeline);
            vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, actor->pipeline->pipelineLayout, 0, 1, &actor->dsInfo->descriptorSets[currentFrame]->descriptorSet, 0, nullptr);
            vkCmdSetViewport(commandBuffer, 0, 1, &viewport);
            vkCmdSetScissor(commandBuffer, 0, 1, &scissor);

            VkDeviceSize offsets[] = {0};
            vkCmdBindVertexBuffers(commandBuffer, 0, 1, &model->vertBuf.buf, offsets);
            vkCmdBindIndexBuffer(commandBuffer, model->indexBuf.buf, 0, VK_INDEX_TYPE_UINT32);
            vkCmdDrawIndexed(commandBuffer, model->numIndices, 1, 0, 0, 0);
        }
    }

    VulkCamera &getCamera()
    {
        return scene->camera;
    }

    ~World() {}
};
