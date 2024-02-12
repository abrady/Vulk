#pragma once

#include "VulkUtil.h"
#include "Vulk.h"
#include "VulkDescriptorPoolBuilder.h"
#include "VulkUniformBuffer.h"
#include "VulkDescriptorSetLayoutBuilder.h"
#include "VulkDescriptorSetUpdater.h"
#include "Common/ClassNonCopyableNonMovable.h"
#include "VulkFrameUBOs.h"
#include "VulkSampler.h"
#include "VulkDescriptorSet.h"

class VulkTextureView;

struct VulkDescriptorSetInfo : public ClassNonCopyableNonMovable
{
    Vulk &vk;
    VkDescriptorSetLayout descriptorSetLayout;
    VkDescriptorPool descriptorPool;
    std::array<std::shared_ptr<VulkDescriptorSet>, MAX_FRAMES_IN_FLIGHT> descriptorSets;

    VulkDescriptorSetInfo(Vulk &vk, VkDescriptorSetLayout descriptorSetLayout, VkDescriptorPool descriptorPool, std::array<std::shared_ptr<VulkDescriptorSet>, MAX_FRAMES_IN_FLIGHT> &&descriptorSets)
        : vk(vk),
          descriptorSetLayout(descriptorSetLayout),
          descriptorPool(descriptorPool),
          descriptorSets(std::move(descriptorSets)) {}

    ~VulkDescriptorSetInfo()
    {
        vkDestroyDescriptorPool(vk.device, descriptorPool, nullptr);
        vkDestroyDescriptorSetLayout(vk.device, descriptorSetLayout, nullptr);
    }
};

class VulkDescriptorSetBuilder
{
    Vulk &vk;
    VulkDescriptorSetLayoutBuilder layoutBuilder;
    VulkDescriptorPoolBuilder poolBuilder;
    struct BufSetUpdaterInfo
    {
        VkBuffer buf;
        VkDeviceSize range;
    };

    struct PerFrameInfo
    {
        std::unordered_map<VulkShaderUBOBindings, BufSetUpdaterInfo> uniformSetInfos;
        std::unordered_map<VulkShaderSSBOBindings, BufSetUpdaterInfo> ssboSetInfos;
    };
    std::array<PerFrameInfo, MAX_FRAMES_IN_FLIGHT> perFrameInfos;

    struct SamplerSetUpdaterInfo
    {
        std::shared_ptr<VulkTextureView> imageView;
        std::shared_ptr<VulkSampler> sampler;
    };
    std::unordered_map<VulkShaderTextureBindings, SamplerSetUpdaterInfo> samplerSetInfos;

public:
    VulkDescriptorSetBuilder(Vulk &vk) : vk(vk), layoutBuilder(vk), poolBuilder(vk) {}

    template <typename T>
    VulkDescriptorSetBuilder &addFrameUBOs(VulkFrameUBOs<T> const &ubos, VkShaderStageFlagBits stageFlags, VulkShaderUBOBindings bindingID)
    {
        layoutBuilder.addUniformBuffer(stageFlags, bindingID);
        poolBuilder.addUniformBufferCount(MAX_FRAMES_IN_FLIGHT);
        for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
        {
            perFrameInfos[i].uniformSetInfos[bindingID] = {ubos.bufs[i], sizeof(T)};
        }
        return *this;
    }

    // for non-mutable uniform buffers
    template <typename T>
    VulkDescriptorSetBuilder &addUniformBuffer(VulkUniformBuffer<T> const &uniformBuffer, VkShaderStageFlagBits stageFlags, VulkShaderUBOBindings bindingID)
    {
        layoutBuilder.addUniformBuffer(stageFlags, bindingID);
        poolBuilder.addUniformBufferCount(MAX_FRAMES_IN_FLIGHT);
        for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
        {
            perFrameInfos[i].uniformSetInfos[bindingID] = {uniformBuffer.buf, sizeof(T)};
        }
        return *this;
    }

    VulkDescriptorSetBuilder &addImageSampler(VkShaderStageFlags stageFlags, VulkShaderTextureBindings bindingID, std::shared_ptr<VulkTextureView> imageView, std::shared_ptr<VulkSampler> sampler)
    {
        layoutBuilder.addImageSampler(stageFlags, bindingID);
        poolBuilder.addCombinedImageSamplerCount(MAX_FRAMES_IN_FLIGHT);
        samplerSetInfos[bindingID] = {imageView, sampler};
        return *this;
    }

    std::shared_ptr<VulkDescriptorSetInfo> build()
    {
        std::shared_ptr<VulkDescriptorSetLayout> descriptorSetLayout = layoutBuilder.build();
        VkDescriptorPool pool = poolBuilder.build(MAX_FRAMES_IN_FLIGHT);
        std::array<std::shared_ptr<VulkDescriptorSet>, MAX_FRAMES_IN_FLIGHT> descriptorSets;
        for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
        {
            descriptorSets[i] = std::make_shared<VulkDescriptorSet>(vk, descriptorSetLayout->layout, pool);
            VulkDescriptorSetUpdater updater = VulkDescriptorSetUpdater(descriptorSets[i]);

            for (auto &pair : perFrameInfos[i].uniformSetInfos)
            {
                updater.addUniformBuffer(pair.second.buf, pair.second.range, pair.first);
            }
            for (auto &pair : perFrameInfos[i].ssboSetInfos)
            {
                updater.addStorageBuffer(pair.second.buf, pair.second.range, pair.first);
            }
            for (auto &pair : samplerSetInfos)
            {
                updater.addImageSampler(pair.second.imageView, pair.second.sampler, pair.first);
            }

            updater.update(vk.device);
        }
        // crappy ownership pass.
        VkDescriptorSetLayout layout = descriptorSetLayout->layout;
        descriptorSetLayout->layout = VK_NULL_HANDLE;
        return std::make_shared<VulkDescriptorSetInfo>(vk, layout, pool, std::move(descriptorSets));
    }
};