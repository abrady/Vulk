#pragma once

#include "Vulk/Vulk.h"
#include "Vulk/VulkActor.h"
#include "Vulk/VulkBufferBuilder.h"
#include "Vulk/VulkCamera.h"
#include "Vulk/VulkDescriptorPoolBuilder.h"
#include "Vulk/VulkDescriptorSetBuilder.h"
#include "Vulk/VulkDescriptorSetUpdater.h"
#include "Vulk/VulkGeo.h"
#include "Vulk/VulkPipelineBuilder.h"
#include "Vulk/VulkResourceMetadata.h"
#include "Vulk/VulkResources.h"
#include "Vulk/VulkScene.h"
#include "Vulk/VulkStorageBuffer.h"
#include "Vulk/VulkUniformBuffer.h"

class World {
  public:
    Vulk &vk;
    std::shared_ptr<VulkScene> scene;
    std::shared_ptr<VulkPipeline> debugNormalsPipeline;
    std::vector<std::shared_ptr<VulkActor>> debugNormalsActors;
    std::vector<std::shared_ptr<VulkActor>> debugTangentsActors;

    struct Debug {
        bool renderNormals = true;
        bool renderTangents = true;
    } debug;

  public:
    World(Vulk &vk, std::string sceneName) : vk(vk) {
        VulkResources resources(vk);
        resources.loadScene(sceneName);
        scene = resources.scenes[sceneName];
        debugNormalsPipeline = resources.getPipeline("DebugNormals");
        auto debugNormalsPipelineDef = resources.metadata.pipelines.at("DebugNormals");
        auto debugTangentsPipelineDef = resources.metadata.pipelines.at("DebugTangents");

        SceneDef &sceneDef = *resources.metadata.scenes.at(sceneName);
        for (int i = 0; i < scene->actors.size(); ++i) {
            auto actor = scene->actors[i];
            auto actorDef = sceneDef.actors[i];
            std::shared_ptr<VulkActor> debugNormalsActor = resources.createActorFromPipeline(*actorDef, debugNormalsPipelineDef, scene);
            debugNormalsActors.push_back(debugNormalsActor);
            std::shared_ptr<VulkActor> debugTangentsActor = resources.createActorFromPipeline(*actorDef, debugTangentsPipelineDef, scene);
            debugTangentsActors.push_back(debugTangentsActor);
        }
    }

    VulkPauseableTimer rotateWorldTimer;
    void updateXformsUBO(VulkSceneUBOs::XformsUBO &ubo, VkViewport const &viewport) {
        float time = rotateWorldTimer.getElapsedTime();
        ubo.world = glm::rotate(glm::mat4(1.0f), time * glm::radians(90.0f), glm::vec3(0.0f, 1.0f, 0.0f));
        glm::vec3 fwd = scene->camera.getForwardVec();
        glm::vec3 lookAt = scene->camera.eye + fwd;
        glm::vec3 up = scene->camera.getUpVec();
        ubo.view = glm::lookAt(scene->camera.eye, lookAt, up);
        ubo.proj = glm::perspective(glm::radians(45.0f), viewport.width / (float)viewport.height, scene->camera.nearClip, scene->camera.farClip);
        ubo.proj[1][1] *= -1;
    }

    void render(VkCommandBuffer commandBuffer, uint32_t currentFrame, VkViewport const &viewport, VkRect2D const &scissor) {
        updateXformsUBO(*scene->sceneUBOs.xforms.ptrs[currentFrame], viewport);
        *scene->sceneUBOs.eyePos.ptrs[currentFrame] = scene->camera.eye;

        // render the scene?
        for (auto &actor : scene->actors) {
            auto model = actor->model;
            vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, actor->pipeline->pipeline);
            vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, actor->pipeline->pipelineLayout, 0, 1,
                                    &actor->dsInfo->descriptorSets[currentFrame]->descriptorSet, 0, nullptr);
            vkCmdSetViewport(commandBuffer, 0, 1, &viewport);
            vkCmdSetScissor(commandBuffer, 0, 1, &scissor);

            VkDeviceSize offsets[] = {0};
            vkCmdBindVertexBuffers(commandBuffer, 0, 1, &model->vertBuf.buf, offsets);
            vkCmdBindIndexBuffer(commandBuffer, model->indexBuf.buf, 0, VK_INDEX_TYPE_UINT32);
            vkCmdDrawIndexed(commandBuffer, model->numIndices, 1, 0, 0, 0);
        }

        // render the debug normals
        if (debug.renderNormals) {
            scene->debugNormalsUBO->mappedUBO->useModel = false;
            for (auto &actor : debugNormalsActors) {
                auto model = actor->model;
                vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, actor->pipeline->pipeline);
                vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, actor->pipeline->pipelineLayout, 0, 1,
                                        &actor->dsInfo->descriptorSets[currentFrame]->descriptorSet, 0, nullptr);
                vkCmdSetViewport(commandBuffer, 0, 1, &viewport);
                vkCmdSetScissor(commandBuffer, 0, 1, &scissor);

                VkDeviceSize offsets[] = {0};
                vkCmdBindVertexBuffers(commandBuffer, 0, 1, &model->vertBuf.buf, offsets);
                vkCmdDraw(commandBuffer, model->numVertices, 1, 0, 0);
            }
        }

        // render the debug tangents
        if (debug.renderTangents) {
            scene->debugTangentsUBO->mappedUBO->length = .1f;
            for (auto &actor : debugTangentsActors) {
                auto model = actor->model;
                vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, actor->pipeline->pipeline);
                vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, actor->pipeline->pipelineLayout, 0, 1,
                                        &actor->dsInfo->descriptorSets[currentFrame]->descriptorSet, 0, nullptr);
                vkCmdSetViewport(commandBuffer, 0, 1, &viewport);
                vkCmdSetScissor(commandBuffer, 0, 1, &scissor);

                VkDeviceSize offsets[] = {0};
                vkCmdBindVertexBuffers(commandBuffer, 0, 1, &model->vertBuf.buf, offsets);
                vkCmdDraw(commandBuffer, model->numVertices, 1, 0, 0);
            }
        }
    }

    bool keyCallback(int key, int /*scancode*/, int action, int /*mods*/) {
        VulkCamera &camera = scene->camera;
        bool handled = false;
        if (action == GLFW_PRESS || action == GLFW_REPEAT) {
            glm::vec3 fwd = camera.getForwardVec();
            glm::vec3 right = camera.getRightVec();
            glm::vec3 up = camera.getUpVec();
            handled = true;
            float move = .2f;
            if (key == GLFW_KEY_W)
                camera.eye += move * fwd;
            else if (key == GLFW_KEY_A)
                camera.eye -= move * right;
            else if (key == GLFW_KEY_S)
                camera.eye -= move * fwd;
            else if (key == GLFW_KEY_D)
                camera.eye += move * right;
            else if (key == GLFW_KEY_Q)
                camera.eye -= move * up;
            else if (key == GLFW_KEY_E)
                camera.eye += move * up;
            else if (key == GLFW_KEY_LEFT)
                camera.yaw -= 15.0f;
            else if (key == GLFW_KEY_RIGHT)
                camera.yaw += 15.0f;
            else if (key == GLFW_KEY_UP)
                camera.pitch += 15.0f;
            else if (key == GLFW_KEY_DOWN)
                camera.pitch -= 15.0f;
            else if (key == GLFW_KEY_SPACE)
                rotateWorldTimer.toggle();
            else
                handled = false;
            if (handled) {
                camera.yaw = fmodf(camera.yaw, 360.0f);
                camera.pitch = fmodf(camera.pitch, 360.0f);
                std::cout << "eye: " << camera.eye.x << ", " << camera.eye.y << ", " << camera.eye.z << " yaw: " << camera.yaw << " pitch: " << camera.pitch
                          << std::endl;
                return true;
            }
        }
        return false;
    }

    ~World() {
    }
};
