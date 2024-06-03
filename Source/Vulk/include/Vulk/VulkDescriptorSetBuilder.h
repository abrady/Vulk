#pragma once

#include "ClassNonCopyableNonMovable.h"
#include "Vulk.h"
#include "VulkDescriptorPoolBuilder.h"
#include "VulkDescriptorSet.h"
#include "VulkDescriptorSetLayoutBuilder.h"
#include "VulkDescriptorSetUpdater.h"
#include "VulkFrameUBOs.h"
#include "VulkSampler.h"
#include "VulkUniformBuffer.h"
#include "VulkUtil.h"

class VulkImageView;
class VulkDescriptorSetLayout;

class VulkDescriptorSetInfo : public ClassNonCopyableNonMovable {
    Vulk& vk;

public:
    // These need to be kept around as long as the descriptor set is in use
    std::shared_ptr<VulkDescriptorSetLayout> descriptorSetLayout;
    VkDescriptorPool descriptorPool;

    std::array<std::shared_ptr<VulkDescriptorSet>, MAX_FRAMES_IN_FLIGHT> descriptorSets;

    VulkDescriptorSetInfo(Vulk& vk, std::shared_ptr<VulkDescriptorSetLayout> descriptorSetLayout, VkDescriptorPool descriptorPool,
                          std::array<std::shared_ptr<VulkDescriptorSet>, MAX_FRAMES_IN_FLIGHT>&& descriptorSets)
        : vk(vk)
        , descriptorSetLayout(descriptorSetLayout)
        , descriptorPool(descriptorPool)
        , descriptorSets(std::move(descriptorSets)) {}

    ~VulkDescriptorSetInfo() {
        vkDestroyDescriptorPool(vk.device, descriptorPool, nullptr);
    }
};

class VulkDescriptorSetBuilder {
    Vulk& vk;
    VulkDescriptorSetLayoutBuilder layoutBuilder;
    VulkDescriptorPoolBuilder poolBuilder;
    std::shared_ptr<VulkDescriptorSetLayout> descriptorSetLayoutOverride;
    struct BufSetUpdaterInfo {
        VkBuffer buf;
        VkDeviceSize range;
    };

    struct PerFrameInfo {
        std::unordered_map<vulk::VulkShaderUBOBinding::type, BufSetUpdaterInfo> uniformSetInfos;
        std::unordered_map<vulk::VulkShaderSSBOBinding::type, BufSetUpdaterInfo> ssboSetInfos;
    };
    std::array<PerFrameInfo, MAX_FRAMES_IN_FLIGHT> perFrameInfos;

    struct SamplerSetUpdaterInfo {
        std::shared_ptr<VulkImageView> imageView;
        std::shared_ptr<VulkSampler> sampler;
    };
    std::array<std::unordered_map<vulk::VulkShaderTextureBinding::type, SamplerSetUpdaterInfo>, MAX_FRAMES_IN_FLIGHT> perFrameSamplerSetInfos;

public:
    VulkDescriptorSetBuilder(Vulk& vk)
        : vk(vk)
        , layoutBuilder(vk)
        , poolBuilder(vk) {}

    // if we have this cached and it matches the current layout just use it. make sure you know what you're doing
    VulkDescriptorSetBuilder& setDescriptorSetLayout(std::shared_ptr<VulkDescriptorSetLayout> descriptorSetLayout) {
        this->descriptorSetLayoutOverride = descriptorSetLayout;
        return *this;
    }

    template <typename T>
    VulkDescriptorSetBuilder& addFrameUBOs(VulkFrameUBOs<T> const& ubos, VkShaderStageFlagBits stageFlags, vulk::VulkShaderUBOBinding::type bindingID) {
        layoutBuilder.addUniformBuffer(stageFlags, bindingID);
        poolBuilder.addUniformBufferCount(MAX_FRAMES_IN_FLIGHT);
        for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
            perFrameInfos[i].uniformSetInfos[bindingID] = {ubos.bufs[i], sizeof(T)};
        }
        return *this;
    }

    // for non-mutable uniform buffers
    template <typename T>
    VulkDescriptorSetBuilder& addUniformBuffer(VulkUniformBuffer<T> const& uniformBuffer, VkShaderStageFlagBits stageFlags, vulk::VulkShaderUBOBinding::type bindingID) {
        layoutBuilder.addUniformBuffer(stageFlags, bindingID);
        poolBuilder.addUniformBufferCount(MAX_FRAMES_IN_FLIGHT);
        for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
            perFrameInfos[i].uniformSetInfos[bindingID] = {uniformBuffer.buf, sizeof(T)};
        }
        return *this;
    }

    // for non-mutable image views that are the same for both frames.
    VulkDescriptorSetBuilder& addBothFramesImageSampler(VkShaderStageFlags stageFlags, vulk::VulkShaderTextureBinding::type bindingID, std::shared_ptr<VulkImageView> imageView,
                                                        std::shared_ptr<VulkSampler> sampler) {
        layoutBuilder.addImageSampler(stageFlags, bindingID);
        poolBuilder.addCombinedImageSamplerCount(MAX_FRAMES_IN_FLIGHT);
        perFrameSamplerSetInfos[0][bindingID] = {imageView, sampler};
        perFrameSamplerSetInfos[1][bindingID] = {imageView, sampler};
        return *this;
    }

    VulkDescriptorSetBuilder& addFrameImageSampler(uint32_t frame, VkShaderStageFlags stageFlags, vulk::VulkShaderTextureBinding::type bindingID,
                                                   std::shared_ptr<VulkImageView> imageView, std::shared_ptr<VulkSampler> sampler) {
        layoutBuilder.addImageSampler(stageFlags, bindingID);
        poolBuilder.addCombinedImageSamplerCount(1);
        perFrameSamplerSetInfos[frame][bindingID] = {imageView, sampler};
        return *this;
    }

    std::shared_ptr<VulkDescriptorSetInfo> build() {
        std::shared_ptr<VulkDescriptorSetLayout> descriptorSetLayout;
        if (this->descriptorSetLayoutOverride) {
            descriptorSetLayout = this->descriptorSetLayoutOverride;
        } else {
            descriptorSetLayout = layoutBuilder.build();
        }
        VkDescriptorPool pool = poolBuilder.build(MAX_FRAMES_IN_FLIGHT);
        std::array<std::shared_ptr<VulkDescriptorSet>, MAX_FRAMES_IN_FLIGHT> descriptorSets;
        for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
            descriptorSets[i] = std::make_shared<VulkDescriptorSet>(vk, descriptorSetLayout->layout, pool);
            VulkDescriptorSetUpdater updater = VulkDescriptorSetUpdater(descriptorSets[i]);

            for (auto& pair : perFrameInfos[i].uniformSetInfos) {
                updater.addUniformBuffer(pair.second.buf, pair.second.range, pair.first);
            }
            for (auto& pair : perFrameInfos[i].ssboSetInfos) {
                updater.addStorageBuffer(pair.second.buf, pair.second.range, pair.first);
            }
            for (auto& pair : perFrameSamplerSetInfos[i]) {
                updater.addImageSampler(pair.second.imageView, pair.second.sampler, pair.first);
            }

            updater.update(vk.device);
        }
        return std::make_shared<VulkDescriptorSetInfo>(vk, descriptorSetLayout, pool, std::move(descriptorSets));
    }
};