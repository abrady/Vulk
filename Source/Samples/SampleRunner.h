#pragma once

#include <Vulk/Vulk.h>

template <typename T>
class SampleRunner : public Vulk
{
public:
    SampleRunner(std::string scene) : scene(scene) {}

protected:
    std::unique_ptr<T> world;
    std::string scene;
    void init() override
    {
        world = std::make_unique<T>(*this, scene);
    }

    void drawFrame(VkCommandBuffer commandBuffer, VkFramebuffer frameBuffer) override
    {
        VkCommandBufferBeginInfo beginInfo{};
        beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

        VK_CALL(vkBeginCommandBuffer(commandBuffer, &beginInfo));

        VkRenderPassBeginInfo renderPassInfo{};
        renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
        renderPassInfo.renderPass = renderPass;
        renderPassInfo.framebuffer = frameBuffer;
        renderPassInfo.renderArea.offset = {0, 0};
        renderPassInfo.renderArea.extent = swapChainExtent;

        std::array<VkClearValue, 2> clearValues{};
        clearValues[0].color = {{0.1f, 0.0f, 0.1f, 1.0f}};
        clearValues[1].depthStencil = {1.0f, 0};

        renderPassInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
        renderPassInfo.pClearValues = clearValues.data();

        VkViewport viewport{};
        viewport.x = 0.0f;
        viewport.y = 0.0f;
        viewport.width = (float)swapChainExtent.width;
        viewport.height = (float)swapChainExtent.height;
        viewport.minDepth = 0.0f;
        viewport.maxDepth = 1.0f;

        VkRect2D scissor{};
        scissor.offset = {0, 0};
        scissor.extent = swapChainExtent;

        vkCmdBeginRenderPass(commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
        {
            world->render(commandBuffer, currentFrame, viewport, scissor);
        }
        vkCmdEndRenderPass(commandBuffer);

        VK_CALL(vkEndCommandBuffer(commandBuffer));
    }

    virtual void cleanup()
    {
        world.reset();
    }

    void keyCallback(int key, int scancode, int action, int mods)
    {
        if (!world->keyCallback(key, scancode, action, mods))
            Vulk::keyCallback(key, scancode, action, mods);
    }
};
