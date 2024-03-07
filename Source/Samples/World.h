#pragma once

#include <memory>

#include "Vulk/Vulk.h"
#include "Vulk/VulkActor.h"
#include "Vulk/VulkBufferBuilder.h"
#include "Vulk/VulkCamera.h"
#include "Vulk/VulkDepthRenderpass.h"
#include "Vulk/VulkDescriptorPoolBuilder.h"
#include "Vulk/VulkDescriptorSetBuilder.h"
#include "Vulk/VulkDescriptorSetUpdater.h"
#include "Vulk/VulkFence.h"
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

    std::shared_ptr<VulkDepthRenderpass> shadowMapRenderpass;
    std::vector<std::shared_ptr<VulkActor>> shadowMapActors;
    std::shared_ptr<VulkPipeline> shadowMapPipeline;
    std::shared_ptr<VulkFence> shadowMapFence;

    std::shared_ptr<VulkPipeline> debugNormalsPipeline;
    std::vector<std::shared_ptr<VulkActor>> debugTangentsActors;
    std::vector<std::shared_ptr<VulkActor>> debugNormalsActors;

    struct Debug {
        bool renderNormals = false;
        bool renderTangents = false;
    } debug;

  public:
    World(Vulk &vk, std::string sceneName) : vk(vk) {
        shadowMapRenderpass = std::make_shared<VulkDepthRenderpass>(vk);
        shadowMapFence = std::make_shared<VulkFence>(vk);

        VulkResources resources(vk);
        resources.loadScene(vk.renderPass, sceneName, shadowMapRenderpass->depthViews);
        scene = resources.scenes[sceneName];
        SceneDef &sceneDef = *resources.metadata.scenes.at(sceneName);

        shadowMapPipeline = resources.loadPipeline(shadowMapRenderpass->renderPass, "ShadowMap");
        auto shadowMapPipelineDef = resources.metadata.pipelines.at("ShadowMap");
        for (int i = 0; i < scene->actors.size(); ++i) {
            auto actor = scene->actors[i];
            auto actorDef = sceneDef.actors[i];
            std::shared_ptr<VulkActor> shadowMapActor = resources.createActorFromPipeline(*actorDef, shadowMapPipelineDef, scene);
            shadowMapActors.push_back(shadowMapActor);
        }

        // ========================================================================================================
        // Debug stuff

        // always create the debug actors/pipeline so we can render them on command.
        debugNormalsPipeline = resources.loadPipeline(vk.renderPass, "DebugNormals");
        resources.loadPipeline(vk.renderPass, "DebugTangents"); // TODO: does this even need to be a separate pipeline?
        auto debugNormalsPipelineDef = resources.metadata.pipelines.at("DebugNormals");
        auto debugTangentsPipelineDef = resources.metadata.pipelines.at("DebugTangents");

        // could probably defer this until we actually want to render the debug normals, eh.
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
    void updateXformsUBO(VkCommandBuffer commandBuffer, VulkSceneUBOs::XformsUBO &ubo, VkViewport const &viewport, glm::mat4 lookAt, float nearClip = 0.1f,
                         float farClip = 100.0f) {
        float rotationTime = rotateWorldTimer.getElapsedTime(); // make sure this stays the same for the entire frame
        ubo.world = glm::rotate(glm::mat4(1.0f), rotationTime * glm::radians(90.0f), glm::vec3(0.0f, 1.0f, 0.0f));
        ubo.view = lookAt;
        ubo.proj = glm::perspective(glm::radians(45.0f), viewport.width / (float)viewport.height, nearClip, farClip);
        ubo.proj[1][1] *= -1;
    }

    void drawFrame(VkCommandBuffer commandBuffer, VkFramebuffer frameBuffer) {
        // set up the global ubos
        // - the xforms which is just to say the world, view, and proj matrices
        // - the actor local transforms (if necessary, not doing this currently)
        // - the eyePos
        // - the lightViewProj : both for shadow map and for sampling during the main render

        VkViewport viewport{};
        viewport.x = 0.0f;
        viewport.y = 0.0f;
        viewport.width = (float)vk.swapChainExtent.width;
        viewport.height = (float)vk.swapChainExtent.height;
        viewport.minDepth = 0.0f;
        viewport.maxDepth = 1.0f;

        glm::vec3 fwd = scene->camera.getForwardVec();
        glm::vec3 lookAt = scene->camera.eye + fwd;
        glm::vec3 up = scene->camera.getUpVec();
        *scene->sceneUBOs.eyePos.ptrs[vk.currentFrame] = scene->camera.eye;

        updateXformsUBO(commandBuffer, *scene->sceneUBOs.xforms.ptrs[vk.currentFrame], viewport, glm::lookAt(scene->camera.eye, lookAt, up),
                        scene->camera.nearClip, scene->camera.farClip);

        glm::vec3 camFwd = scene->camera.getForwardVec();
        glm::vec3 focusPt = scene->camera.eye + camFwd; // TODO: figure out what this should be
        up = glm::vec3(0.0f, 1.0f, 0.0f);               // TODO: also kinda arbitrary up vec.
        glm::mat4 lightView = glm::lookAt(scene->sceneUBOs.pointLight.mappedUBO->pos, focusPt, up);
        glm::mat4 lightProj = glm::perspective(glm::radians(90.0f), 1.0f, 0.1f, 100.0f);
        glm::mat4 viewProj = lightProj * lightView;
        scene->lightViewProjUBO->mappedUBO->viewProj = viewProj;

        // now render
        VkCommandBufferBeginInfo beginInfo{};
        beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

        std::shared_ptr<VulkTextureView> depthView = shadowMapRenderpass->depthViews[vk.currentFrame]->depthView;

        VK_CALL(vkBeginCommandBuffer(commandBuffer, &beginInfo));
        renderShadowMapImageForLight(commandBuffer, viewport, *scene->sceneUBOs.pointLight.mappedUBO);
        vk.transitionImageLayout(commandBuffer, depthView->image, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
        drawMainStuff(commandBuffer, viewport, frameBuffer);
        vk.transitionImageLayout(commandBuffer, depthView->image, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL);
        VK_CALL(vkEndCommandBuffer(commandBuffer));
    }

    void renderShadowMapImageForLight(VkCommandBuffer commandBuffer, VkViewport const &viewport, VulkPointLight &light) {
        VkClearValue clearColor = {1.0f, 0.0f, 0.0f, 0.0f}; // Use 1.0f for depth clearing
        VkRenderPassBeginInfo renderPassBeginInfo = {};
        renderPassBeginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
        renderPassBeginInfo.renderPass = shadowMapRenderpass->renderPass;
        renderPassBeginInfo.framebuffer = shadowMapRenderpass->frameBuffers[vk.currentFrame];
        renderPassBeginInfo.renderArea.offset = {0, 0};
        renderPassBeginInfo.renderArea.extent = shadowMapRenderpass->extent;
        renderPassBeginInfo.clearValueCount = 1;
        renderPassBeginInfo.pClearValues = &clearColor;

        VkRect2D scissor{};
        scissor.offset = {0, 0};
        scissor.extent = vk.swapChainExtent;

        vkCmdBeginRenderPass(commandBuffer, &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);
        for (auto &actor : shadowMapActors) {
            auto model = actor->model;
            vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, actor->pipeline->pipeline);
            vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, actor->pipeline->pipelineLayout, 0, 1,
                                    &actor->dsInfo->descriptorSets[vk.currentFrame]->descriptorSet, 0, nullptr);
            vkCmdSetViewport(commandBuffer, 0, 1, &viewport);
            vkCmdSetScissor(commandBuffer, 0, 1, &scissor);

            VkDeviceSize offsets[] = {0};
            vkCmdBindVertexBuffers(commandBuffer, 0, 1, &model->vertBuf.buf, offsets);
            vkCmdBindIndexBuffer(commandBuffer, model->indexBuf.buf, 0, VK_INDEX_TYPE_UINT32);
            vkCmdDrawIndexed(commandBuffer, model->numIndices, 1, 0, 0, 0);
        }
        vkCmdEndRenderPass(commandBuffer);
    }

    void drawMainStuff(VkCommandBuffer commandBuffer, VkViewport const &viewport, VkFramebuffer frameBuffer) {
        VkRenderPassBeginInfo renderPassInfo{};
        renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
        renderPassInfo.renderPass = vk.renderPass;
        renderPassInfo.framebuffer = frameBuffer;
        renderPassInfo.renderArea.offset = {0, 0};
        renderPassInfo.renderArea.extent = vk.swapChainExtent;

        std::array<VkClearValue, 2> clearValues{};
        clearValues[0].color = {{0.1f, 0.0f, 0.1f, 1.0f}};
        clearValues[1].depthStencil = {1.0f, 0};

        renderPassInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
        renderPassInfo.pClearValues = clearValues.data();

        VkRect2D scissor{};
        scissor.offset = {0, 0};
        scissor.extent = vk.swapChainExtent;

        vkCmdBeginRenderPass(commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
        render(commandBuffer, vk.currentFrame, viewport, scissor);
        vkCmdEndRenderPass(commandBuffer);
    }

    void render(VkCommandBuffer commandBuffer, uint32_t currentFrame, VkViewport const &viewport, VkRect2D const &scissor) {
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
