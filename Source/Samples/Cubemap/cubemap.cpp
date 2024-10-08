#include <memory>
#include "Vulk/VulkPCH.h"
#include "imgui.h"

class World final : public VulkRenderable {
   public:
    Vulk& vk;
    std::shared_ptr<VulkScene> scene;

    std::shared_ptr<VulkDepthRenderpass> shadowMapRenderpass;
    std::vector<std::shared_ptr<VulkActor>> shadowMapActors;
    std::shared_ptr<VulkPipeline> shadowMapPipeline;
    std::shared_ptr<VulkFence> shadowMapFence;

    std::vector<std::shared_ptr<VulkActor>> debugTangentsActors;
    std::vector<std::shared_ptr<VulkActor>> debugNormalsActors;

    std::shared_ptr<VulkPipeline> wireframePipeline;
    std::vector<std::shared_ptr<VulkActor>> debugWireframeActors;

    std::shared_ptr<VulkPickRenderpass> pickRenderpass;
    std::vector<std::shared_ptr<VulkActor>> pickActors;
    std::shared_ptr<VulkPipeline> pickPipeline;

    std::shared_ptr<VulkActor> axesActor;
    std::shared_ptr<VulkPipeline> axesPipeline;

    struct Debug {
        bool renderNormals   = false;
        bool renderTangents  = false;
        bool renderWireframe = false;
    } debug;

    std::shared_ptr<spdlog::logger> logger = VulkLogger::CreateLogger("World");

   public:
    World(Vulk& vk, std::string projFile) : vk(vk) {
        shadowMapRenderpass = std::make_shared<VulkDepthRenderpass>(vk);
        shadowMapFence      = std::make_shared<VulkFence>(vk);

        vulk::cpp2::ProjectDef projDef;
        readDefFromFile(projFile, projDef);
        std::string sceneName                    = projDef.get_startingScene();
        std::shared_ptr<VulkResources> resources = VulkResources::loadFromProject(vk, projFile);
        resources->loadScene(vk.renderPass, sceneName, shadowMapRenderpass->depthViews);
        SceneDef& sceneDef = *resources->metadata->scenes.at(sceneName);
        scene              = resources->scenes[sceneName];

        shadowMapPipeline = resources->loadPipeline(shadowMapRenderpass->renderPass, shadowMapRenderpass->extent, "ShadowMap");
        auto shadowMapPipelineDef = resources->metadata->pipelines.at("ShadowMap");
        for (size_t i = 0; i < scene->actors.size(); ++i) {
            auto actor                                = scene->actors[i];
            auto actorDef                             = sceneDef.actors[i];
            std::shared_ptr<VulkActor> shadowMapActor = resources->createActorFromPipeline(*actorDef, shadowMapPipeline, scene);
            shadowMapActors.push_back(shadowMapActor);
        }

        pickRenderpass = std::make_shared<VulkPickRenderpass>(vk);
        pickPipeline   = resources->loadPipeline(pickRenderpass->renderPass, vk.swapChainExtent, "Pick");
        for (size_t i = 0; i < scene->actors.size(); ++i) {
            auto actor                           = scene->actors[i];
            auto actorDef                        = sceneDef.actors[i];
            std::shared_ptr<VulkActor> pickActor = resources->createActorFromPipeline(*actorDef, pickPipeline, scene);
            pickActors.push_back(pickActor);
        }

        // ========================================================================================================
        // Debug stuff

        VulkPBRDebugUBO& pbrDebugUBO = *scene->pbrDebugUBO->mappedUBO;
        pbrDebugUBO.isMetallic       = 0;
        pbrDebugUBO.roughness        = 0.5f;
        pbrDebugUBO.diffuse          = 1;
        pbrDebugUBO.specular         = 1;

        wireframePipeline = resources->loadPipeline(vk.renderPass, vk.swapChainExtent, "Wireframe");
        for (size_t i = 0; i < scene->actors.size(); ++i) {
            auto actor                                = scene->actors[i];
            auto actorDef                             = sceneDef.actors[i];
            std::shared_ptr<VulkActor> wireframeActor = resources->createActorFromPipeline(*actorDef, wireframePipeline, scene);
            // scene->actors[i]->pipeline = wireframeActor->pipeline;
            debugWireframeActors.push_back(wireframeActor);
        }

        // show the axes of world coordinates
        std::shared_ptr<VulkMesh> axesMesh = std::make_shared<VulkMesh>();
        makeAxes(1.0f, *axesMesh);
        // VulkSceneUBOs::XformsUBO& axesUBO = *scene->sceneUBOs.xforms;
        axesPipeline = resources->loadPipeline(vk.renderPass, vk.swapChainExtent, "DebugAxes");

        std::vector<vulk::cpp2::VulkShaderLocation> const axesInputs = {
            vulk::cpp2::VulkShaderLocation::Pos,
            vulk::cpp2::VulkShaderLocation::TexCoord,
        };
        std::shared_ptr<VulkModel> axesModel = std::make_shared<VulkModel>(vk, axesMesh, nullptr, nullptr, axesInputs);

        VulkDescriptorSetBuilder dsBuilder(vk);
        dsBuilder.addFrameUBOs(scene->sceneUBOs.xforms, VK_SHADER_STAGE_VERTEX_BIT, vulk::cpp2::VulkShaderUBOBinding::Xforms);
        std::shared_ptr<VulkDescriptorSetInfo> axesDSInfo = dsBuilder.build();
        axesActor = make_shared<VulkActor>(vk, axesModel, nullptr, axesDSInfo, axesPipeline);

        // always create the debug actors/pipeline so we can render them on command.
        // our cubemap sphere doesn't have a normal map, so this errors. we could use vert normals... eh commenting out for now.
        // std::shared_ptr<VulkPipeline> debugNormalsPipeline = resources->loadPipeline(vk.renderPass, vk.swapChainExtent,
        // "DebugNormals"); std::shared_ptr<VulkPipeline> debugTangentsPipeline = resources->loadPipeline(vk.renderPass,
        // vk.swapChainExtent,
        //                                                                               "DebugTangents"); // TODO: does this even
        //                                                                               need to be a separate pipeline?

        // // could probably defer this until we actually want to render the debug normals, eh.
        // for (size_t i = 0; i < scene->actors.size(); ++i) {
        //     auto actor = scene->actors[i];
        //     auto actorDef = sceneDef.actors[i];
        //     std::shared_ptr<VulkActor> debugNormalsActor = resources->createActorFromPipeline(*actorDef, debugNormalsPipeline,
        //     scene); debugNormalsActors.push_back(debugNormalsActor); std::shared_ptr<VulkActor> debugTangentsActor =
        //     resources->createActorFromPipeline(*actorDef, debugTangentsPipeline, scene);
        //     debugTangentsActors.push_back(debugTangentsActor);
        // }
    }

    struct Menu {
        ImGuiWindowFlags windowFlags = 0;
        bool show                    = true;
        bool isOpen                  = false;
    } menu;

    struct Selection {
        uint32_t selectedModelID = 0;
    } selection;
    std::shared_ptr<Selection> selectedActor;

    void tick() override {
        ImGuiIO& io = ImGui::GetIO();

        float dragScale = 0.001f;
        float dx        = -dragScale * io.MouseDelta.x;
        float dy        = dragScale * io.MouseDelta.y;
        float scroll    = ImGui::GetIO().MouseWheel;
        bool camUpdated = false;

        uint32_t mouseIdx        = (uint32_t)(io.MousePos.x + io.MousePos.y * (float)vk.swapChainExtent.width);
        bool modelPicked         = false;
        uint32_t selectedModelID = 0;
        if (mouseIdx < pickRenderpass->pickData.size()) {
            modelPicked     = pickRenderpass->pickData[mouseIdx] > 0;
            selectedModelID = pickRenderpass->pickData[mouseIdx] - 1;
        }

        if (ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {
            if (modelPicked && selectedActor && selectedActor->selectedModelID == selectedModelID) {
                logger->trace("re-clicked on current model: {}", selectedModelID);
            } else if (modelPicked) {
                logger->trace("selected model: {}", selectedModelID);
                selectedActor = std::make_shared<Selection>(Selection{selectedModelID});
            } else {
                logger->trace("clearing selection");
                selectedActor = nullptr;
            }
        } else if (ImGui::IsMouseDragging(ImGuiMouseButton_Left)) {
            // left mouse is rotate around y axis and move +z/-z
            scene->camera.updatePosition(0.0f, 0.0f, dy);
            scene->camera.updateOrientation(dx, 0.0f);
            camUpdated = true;
        } else if (ImGui::IsMouseDragging(ImGuiMouseButton_Middle)) {
            // middle mouse translates in the x/y axis
            scene->camera.updatePosition(-dx, dy, 0.0f);
        } else if (ImGui::IsMouseDragging(ImGuiMouseButton_Right)) {
            // right mouse button is purely for camera control
            scene->camera.updateOrientation(dx, dy);
            camUpdated = true;
        } else if (scroll != 0.0f) {
            // right mouse button + shift is purely for camera control
            float scrollScale = 0.1f;
            scene->camera.updatePosition(0.0f, 0.0f, -scroll * scrollScale);
            camUpdated = true;
        }

        float speed = 0.1f;  // Adjust speed as necessary
        if (io.KeysDown[GLFW_KEY_W]) {
            scene->camera.updatePosition(0.0f, 0.0f, speed);  // Move forward
        }
        if (io.KeysDown[GLFW_KEY_A]) {
            scene->camera.updatePosition(-speed, 0.0f, 0.0f);  // Move left
        }
        if (io.KeysDown[GLFW_KEY_S]) {
            scene->camera.updatePosition(0.0f, 0.0f, -speed);  // Move backward
        }
        if (io.KeysDown[GLFW_KEY_D]) {
            scene->camera.updatePosition(speed, 0.0f, 0.0f);  // Move right
        }

        if (camUpdated) {
            glm::vec3 eulers = scene->camera.getEulers();
            logger->trace("dx: {:.2f} dy: {:.2f} camera yaw: {:.2f} pitch: {:.2f}", dx, dy, eulers.y, eulers.x);
        }

        // a useful demo of a variety of features
        static bool showDemo = false;
        if (showDemo) {
            static bool demoWindowOpen = false;
            ImGui::ShowDemoWindow(&demoWindowOpen);
        }

        const ImGuiViewport* viewport = ImGui::GetMainViewport();
        ImGui::SetNextWindowPos(ImVec2(viewport->WorkPos.x, viewport->WorkPos.y), ImGuiCond_FirstUseEver);
        ImGui::SetNextWindowSize(ImVec2(550, 680), ImGuiCond_FirstUseEver);
        if (!ImGui::Begin("VulkUI Menu", &menu.isOpen, menu.windowFlags)) {
            // Early out if the window is collapsed, as an optimization.
            ImGui::End();
            return;
        }
        // Most "big" widgets share a common width settings by default. See 'Demo->Layout->Widgets Width' for details.
        // e.g. Use 2/3 of the space for widgets and 1/3 for labels (right align)
        // e.g. Leave a fixed amount of width for labels (by passing a negative value), the rest goes to widgets.
        ImGui::PushItemWidth(ImGui::GetFontSize() * -12);
        if (ImGui::CollapsingHeader("Debug", ImGuiTreeNodeFlags_DefaultOpen)) {
            ImGui::Checkbox("Render Normals", &debug.renderNormals);
            ImGui::Checkbox("Render Tangents", &debug.renderTangents);
            ImGui::Checkbox("Render Wireframe", &debug.renderWireframe);
        }

        VulkPBRDebugUBO& pbrDebugUBO = *scene->pbrDebugUBO->mappedUBO;
        ImGui::Text("Material");
        ImGui::RadioButton("Dielectric", &pbrDebugUBO.isMetallic, 0);
        ImGui::SameLine();
        ImGui::RadioButton("Metallic", &pbrDebugUBO.isMetallic, 1);

        ImGui::SliderFloat("Roughness", &pbrDebugUBO.roughness, 0.0f, 1.0f);

        ImGui::Text("Lighting");
        ImGui::Checkbox("Diffuse", (bool*)&pbrDebugUBO.diffuse);
        ImGui::Checkbox("Specular", (bool*)&pbrDebugUBO.specular);

        ImGui::Text("Camera");
        ImGui::InputFloat3("Eye", glm::value_ptr(scene->camera.eye));
        ImGui::InputFloat4("Rot", glm::value_ptr(scene->camera.orientation));

        // ImGui::SliderInt("slider int", &i1, -1, 3);
        // ImGui::SameLine();
        // HelpMarker("CTRL+click to input value.");

        // static float f1 = 0.123f, f2 = 0.0f;
        // ImGui::SliderFloat("slider float (log)", &f2, -10.0f, 10.0f, "%.4f", ImGuiSliderFlags_Logarithmic);

        ImGui::End();
    }

    VulkPauseableTimer rotateWorldTimer;
    void renderFrame(VkCommandBuffer commandBuffer, VkFramebuffer frameBuffer) override {
        // set up the global ubos
        // - the xforms which is just to say the world, view, and proj matrices
        // - the actor local transforms (if necessary, not doing this currently)
        // - the eyePos
        // - the lightViewProj : both for shadow map and for sampling during the main render

        VkViewport viewport{};
        viewport.x        = 0.0f;
        viewport.y        = 0.0f;
        viewport.width    = (float)vk.swapChainExtent.width;
        viewport.height   = (float)vk.swapChainExtent.height;
        viewport.minDepth = 0.0f;
        viewport.maxDepth = 1.0f;

        scene->globalConstantsUBO->mappedUBO->viewportWidth  = viewport.width;
        scene->globalConstantsUBO->mappedUBO->viewportHeight = viewport.height;

        float rotationTime = rotateWorldTimer.getElapsedTime();  // make sure this stays the same for the entire frame
        float nearClip     = scene->camera.nearClip;
        float farClip      = scene->camera.farClip;

        // glm::vec3 lookAt = scene->camera.lookAt;
        // glm::vec3 up = scene->camera.getUpVec();
        *scene->sceneUBOs.eyePos.ptrs[vk.currentFrame] = scene->camera.eye;

        VulkSceneUBOs::XformsUBO& ubo = *scene->sceneUBOs.xforms.ptrs[vk.currentFrame];
        ubo.world = glm::rotate(glm::mat4(1.0f), rotationTime * glm::radians(90.0f), glm::vec3(0.0f, 1.0f, 0.0f));
        // ubo.view = glm::lookAt(scene->camera.eye, lookAt, up);
        ubo.view = scene->camera.getViewMat();
        glm::mat4 clip(1.0f);
        clip[1][1] = -1;  // flip the Y axis
        ubo.proj   = clip * glm::perspective(DEFAULT_FOV_RADS, viewport.width / (float)viewport.height, nearClip, farClip);

        // Vulkan clip space has inverted Y

        // set up the light view proj
        VulkPointLight& light = *scene->sceneUBOs.pointLight.mappedUBO;
        glm::mat4 lightView   = glm::lookAt(light.pos, glm::vec3(0.0f, 0.0f, 0.0f), VIEWSPACE_UP_VEC);
        glm::mat4 lightProj   = glm::perspective(DEFAULT_FOV_RADS, viewport.width / (float)viewport.height, nearClip, farClip);
        glm::mat4 viewProj    = lightProj * lightView;
        if (scene->lightViewProjUBO) {
            scene->lightViewProjUBO->mappedUBO->viewProj = viewProj;
        }

        std::shared_ptr<VulkImageView> depthView = shadowMapRenderpass->depthViews[vk.currentFrame]->depthView;

        renderPickBuffer(commandBuffer);
        renderShadowMapImageForLight(commandBuffer);
        vk.transitionImageLayout(
            commandBuffer,
            depthView->image,
            VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
            VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
        );
        drawMainStuff(commandBuffer, frameBuffer);
        vk.transitionImageLayout(
            commandBuffer,
            depthView->image,
            VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
            VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL
        );
    }

    void renderPickBuffer(VkCommandBuffer commandBuffer) {
        VkRenderPassBeginInfo renderPassBeginInfo{};
        renderPassBeginInfo.sType             = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
        renderPassBeginInfo.renderPass        = pickRenderpass->renderPass;
        renderPassBeginInfo.framebuffer       = pickRenderpass->frameBuffers[vk.currentFrame];
        renderPassBeginInfo.renderArea.offset = {0, 0};
        renderPassBeginInfo.renderArea.extent = vk.swapChainExtent;

        std::array<VkClearValue, 2> clearValues{};
        clearValues[0].color        = {{0}};
        clearValues[1].depthStencil = {1.0f, 0};

        renderPassBeginInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
        renderPassBeginInfo.pClearValues    = clearValues.data();

        vkCmdBeginRenderPass(commandBuffer, &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);
        for (uint32_t i = 0; i < pickActors.size(); ++i) {
            auto& actor   = pickActors[i];
            auto& model   = actor->model;
            uint32_t data = i + 1;
            vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pickPipeline->pipeline);
            vkCmdPushConstants(commandBuffer, pickPipeline->pipelineLayout, VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(data), &data);
            vkCmdBindDescriptorSets(
                commandBuffer,
                VK_PIPELINE_BIND_POINT_GRAPHICS,
                pickPipeline->pipelineLayout,
                0,
                1,
                &actor->dsInfo->descriptorSets[vk.currentFrame]->descriptorSet,
                0,
                nullptr
            );
            model->bindInputBuffers(commandBuffer);
            vkCmdDrawIndexed(commandBuffer, model->numIndices, 1, 0, 0, 0);
        }

        vkCmdEndRenderPass(commandBuffer);
    }

    void onBeforeRender() override {
        pickRenderpass->updatePickDataFromBuffer(vk.lastFrame);
    }

    void renderShadowMapImageForLight(VkCommandBuffer commandBuffer) {
        VkClearValue clearValue;
        clearValue.depthStencil.depth   = 1.0f;
        clearValue.depthStencil.stencil = 0;

        VkRenderPassBeginInfo renderPassBeginInfo = {};
        renderPassBeginInfo.sType                 = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
        renderPassBeginInfo.renderPass            = shadowMapRenderpass->renderPass;
        renderPassBeginInfo.framebuffer           = shadowMapRenderpass->frameBuffers[vk.currentFrame];
        renderPassBeginInfo.renderArea.offset     = {0, 0};
        renderPassBeginInfo.renderArea.extent     = shadowMapRenderpass->extent;
        renderPassBeginInfo.clearValueCount       = 1;
        renderPassBeginInfo.pClearValues          = &clearValue;

        vkCmdBeginRenderPass(commandBuffer, &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);
        for (auto& actor : shadowMapActors) {
            auto model = actor->model;
            vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, actor->pipeline->pipeline);
            vkCmdBindDescriptorSets(
                commandBuffer,
                VK_PIPELINE_BIND_POINT_GRAPHICS,
                actor->pipeline->pipelineLayout,
                0,
                1,
                &actor->dsInfo->descriptorSets[vk.currentFrame]->descriptorSet,
                0,
                nullptr
            );
            model->bindInputBuffers(commandBuffer);
            vkCmdDrawIndexed(commandBuffer, model->numIndices, 1, 0, 0, 0);
        }
        vkCmdEndRenderPass(commandBuffer);
    }

    void drawMainStuff(VkCommandBuffer commandBuffer, VkFramebuffer frameBuffer) {
        VkRenderPassBeginInfo renderPassInfo{};
        renderPassInfo.sType             = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
        renderPassInfo.renderPass        = vk.renderPass;
        renderPassInfo.framebuffer       = frameBuffer;
        renderPassInfo.renderArea.offset = {0, 0};
        renderPassInfo.renderArea.extent = vk.swapChainExtent;

        std::array<VkClearValue, 2> clearValues{};
        clearValues[0].color        = {{0.1f, 0.0f, 0.1f, 1.0f}};
        clearValues[1].depthStencil = {1.0f, 0};

        renderPassInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
        renderPassInfo.pClearValues    = clearValues.data();

        vkCmdBeginRenderPass(commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
        render(commandBuffer, vk.currentFrame);
        vkCmdEndRenderPass(commandBuffer);
    }

    void render(VkCommandBuffer commandBuffer, uint32_t currentFrame) {
        // render the scene
        if (debug.renderWireframe) {
            for (auto& actor : debugWireframeActors) {
                auto model = actor->model;
                vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, wireframePipeline->pipeline);
                vkCmdBindDescriptorSets(
                    commandBuffer,
                    VK_PIPELINE_BIND_POINT_GRAPHICS,
                    wireframePipeline->pipelineLayout,
                    0,
                    1,
                    &actor->dsInfo->descriptorSets[currentFrame]->descriptorSet,
                    0,
                    nullptr
                );
                model->bindInputBuffers(commandBuffer);
                vkCmdDrawIndexed(commandBuffer, model->numIndices, 1, 0, 0, 0);
            }
        } else {
            for (auto& actor : scene->actors) {
                auto model = actor->model;
                vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, actor->pipeline->pipeline);
                vkCmdBindDescriptorSets(
                    commandBuffer,
                    VK_PIPELINE_BIND_POINT_GRAPHICS,
                    actor->pipeline->pipelineLayout,
                    0,
                    1,
                    &actor->dsInfo->descriptorSets[currentFrame]->descriptorSet,
                    0,
                    nullptr
                );
                model->bindInputBuffers(commandBuffer);
                vkCmdDrawIndexed(commandBuffer, model->numIndices, 1, 0, 0, 0);
            }
        }

        // render the debug normals
        if (debug.renderNormals) {
            scene->debugNormalsUBO->mappedUBO->useModel = false;
            for (auto& actor : debugNormalsActors) {
                auto model = actor->model;
                vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, actor->pipeline->pipeline);
                vkCmdBindDescriptorSets(
                    commandBuffer,
                    VK_PIPELINE_BIND_POINT_GRAPHICS,
                    actor->pipeline->pipelineLayout,
                    0,
                    1,
                    &actor->dsInfo->descriptorSets[currentFrame]->descriptorSet,
                    0,
                    nullptr
                );

                model->bindInputBuffers(commandBuffer);
                vkCmdDraw(commandBuffer, model->numVertices, 1, 0, 0);
            }
        }

        // render the debug tangents
        if (debug.renderTangents) {
            scene->debugTangentsUBO->mappedUBO->length = .1f;
            for (auto& actor : debugTangentsActors) {
                auto model = actor->model;
                vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, actor->pipeline->pipeline);
                vkCmdBindDescriptorSets(
                    commandBuffer,
                    VK_PIPELINE_BIND_POINT_GRAPHICS,
                    actor->pipeline->pipelineLayout,
                    0,
                    1,
                    &actor->dsInfo->descriptorSets[currentFrame]->descriptorSet,
                    0,
                    nullptr
                );

                model->bindInputBuffers(commandBuffer);
                vkCmdDraw(commandBuffer, model->numVertices, 1, 0, 0);
            }
        }

        // draw the axes
        vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, axesPipeline->pipeline);
        vkCmdBindDescriptorSets(
            commandBuffer,
            VK_PIPELINE_BIND_POINT_GRAPHICS,
            axesPipeline->pipelineLayout,
            0,
            1,
            &axesActor->dsInfo->descriptorSets[vk.currentFrame]->descriptorSet,
            0,
            nullptr
        );

        axesActor->model->bindInputBuffers(commandBuffer);
        vkCmdDrawIndexed(commandBuffer, axesActor->model->numIndices, 1, 0, 0, 0);
    }

    // TODO: re-wire this up
    bool keyCallback(int key, int /*scancode*/, int action, int /*mods*/) {
        VulkCamera& camera = scene->camera;
        bool handled       = false;
        if (action == GLFW_PRESS || action == GLFW_REPEAT) {
            glm::vec3 fwd   = camera.getForwardVec();
            glm::vec3 right = camera.getRightVec();
            glm::vec3 up    = camera.getUpVec();
            handled         = true;
            float move      = .2f;
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
                camera.updateOrientation(15.0f, 0.0f);
            else if (key == GLFW_KEY_RIGHT)
                camera.updateOrientation(-15.0f, 0.0f);
            else if (key == GLFW_KEY_UP)
                camera.updateOrientation(0.0f, 15.0f);
            else if (key == GLFW_KEY_DOWN)
                camera.updateOrientation(0.0f, -15.0f);
            else if (key == GLFW_KEY_SPACE)
                rotateWorldTimer.toggle();
            else if (key == GLFW_KEY_N) {
                debug.renderNormals = !debug.renderNormals;
                std::cout << "render normals: " << debug.renderNormals << std::endl;
            } else if (key == GLFW_KEY_T) {
                debug.renderTangents = !debug.renderTangents;
                std::cout << "render tangents: " << debug.renderTangents << std::endl;
            } else
                handled = false;
            if (handled) {
                glm::vec3 eulers = camera.getEulers();
                logger->info("eye: {} yaw: {} pitch: {}", glm::to_string(camera.eye), eulers.y, eulers.x);
                return true;
            }
        }
        return false;
    }

    ~World() {}
};

int main() {
    Vulk app;
    app.renderable = std::make_shared<World>(app, "Cubemap.proj");
    app.run();
    return 0;
}
