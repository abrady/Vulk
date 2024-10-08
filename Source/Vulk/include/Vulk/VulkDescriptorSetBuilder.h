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
    std::shared_ptr<const VulkDescriptorSetLayout> descriptorSetLayout;
    VkDescriptorPool descriptorPool;

    std::array<std::shared_ptr<const VulkDescriptorSet>, MAX_FRAMES_IN_FLIGHT> descriptorSets;

    VulkDescriptorSetInfo(Vulk& vk,
                          std::shared_ptr<const VulkDescriptorSetLayout> descriptorSetLayout,
                          VkDescriptorPool descriptorPool,
                          std::array<std::shared_ptr<const VulkDescriptorSet>, MAX_FRAMES_IN_FLIGHT>&& descriptorSets)
        : vk(vk),
          descriptorSetLayout(descriptorSetLayout),
          descriptorPool(descriptorPool),
          descriptorSets(std::move(descriptorSets)) {}

    ~VulkDescriptorSetInfo() {
        vkDestroyDescriptorPool(vk.device, descriptorPool, nullptr);
    }
};

class VulkDescriptorSetBuilder {
    Vulk& vk;
    VulkDescriptorSetLayoutBuilder layoutBuilder;
    VulkDescriptorPoolBuilder poolBuilder;
    std::shared_ptr<const VulkDescriptorSetLayout> descriptorSetLayoutOverride;
    struct BufSetUpdaterInfo {
        VkBuffer buf;
        VkDeviceSize range;
    };

    struct PerFrameInfo {
        std::unordered_map<vulk::cpp2::VulkShaderUBOBinding, BufSetUpdaterInfo> uniformSetInfos;
        std::unordered_map<vulk::cpp2::VulkShaderSSBOBinding, BufSetUpdaterInfo> ssboSetInfos;
    };
    std::array<PerFrameInfo, MAX_FRAMES_IN_FLIGHT> perFrameInfos;

    struct SamplerSetUpdaterInfo {
        std::shared_ptr<const VulkImageView> imageView;
        std::shared_ptr<const VulkSampler> sampler;
    };
    std::array<std::unordered_map<vulk::cpp2::VulkShaderTextureBinding, SamplerSetUpdaterInfo>, MAX_FRAMES_IN_FLIGHT>
        perFrameSamplerSetInfos;

    struct InputAttachmentInfo {
        // uint32_t atmtIdx;
        std::shared_ptr<const VulkImageView> imageView;
    };
    std::unordered_map<vulk::cpp2::GBufBinding, InputAttachmentInfo> inputAttachments;

   public:
    VulkDescriptorSetBuilder(Vulk& vk) : vk(vk), layoutBuilder(vk), poolBuilder(vk) {}

    // if we have this cached and it matches the current layout just use it. make sure you know what you're doing
    VulkDescriptorSetBuilder& setDescriptorSetLayout(std::shared_ptr<const VulkDescriptorSetLayout> descriptorSetLayout) {
        VULK_ASSERT(descriptorSetLayout);
        this->descriptorSetLayoutOverride = descriptorSetLayout;
        return *this;
    }

    template <typename T>
    VulkDescriptorSetBuilder& addFrameUBOs(VulkFrameUBOs<T> const& ubos,
                                           VkShaderStageFlagBits stageFlags,
                                           vulk::cpp2::VulkShaderUBOBinding bindingID) {
        layoutBuilder.addUniformBuffer(stageFlags, bindingID);
        poolBuilder.addUniformBufferCount(MAX_FRAMES_IN_FLIGHT);
        for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
            perFrameInfos[i].uniformSetInfos[bindingID] = {ubos.bufs[i], sizeof(T)};
        }
        return *this;
    }

    // for non-mutable uniform buffers
    template <typename T>
    VulkDescriptorSetBuilder& addUniformBuffer(VulkUniformBuffer<T> const& uniformBuffer,
                                               VkShaderStageFlagBits stageFlags,
                                               vulk::cpp2::VulkShaderUBOBinding bindingID) {
        layoutBuilder.addUniformBuffer(stageFlags, bindingID);
        poolBuilder.addUniformBufferCount(MAX_FRAMES_IN_FLIGHT);
        for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
            perFrameInfos[i].uniformSetInfos[bindingID] = {uniformBuffer.buf, sizeof(T)};
        }
        return *this;
    }

    // for non-mutable image views that are the same for both frames.
    VulkDescriptorSetBuilder& addBothFramesImageSampler(VkShaderStageFlags stageFlags,
                                                        vulk::cpp2::VulkShaderTextureBinding bindingID,
                                                        std::shared_ptr<const VulkImageView> imageView,
                                                        std::shared_ptr<const VulkSampler> sampler) {
        VULK_ASSERT(imageView && sampler);
        layoutBuilder.addImageSampler(stageFlags, bindingID);
        poolBuilder.addCombinedImageSamplerCount(MAX_FRAMES_IN_FLIGHT);
        perFrameSamplerSetInfos[0][bindingID] = {imageView, sampler};
        perFrameSamplerSetInfos[1][bindingID] = {imageView, sampler};
        return *this;
    }

    VulkDescriptorSetBuilder& addFrameImageSampler(uint32_t frame,
                                                   VkShaderStageFlags stageFlags,
                                                   vulk::cpp2::VulkShaderTextureBinding bindingID,
                                                   std::shared_ptr<const VulkImageView> imageView,
                                                   std::shared_ptr<const VulkSampler> sampler) {
        VULK_ASSERT(imageView && sampler);
        layoutBuilder.addImageSampler(stageFlags, bindingID);
        poolBuilder.addCombinedImageSamplerCount(1);
        perFrameSamplerSetInfos[frame][bindingID] = {imageView, sampler};
        return *this;
    }

    VulkDescriptorSetBuilder& addInputAttachment(VkShaderStageFlags stageFlags,
                                                 auto bindingID,
                                                 std::shared_ptr<const VulkImageView> imageView)
        requires InputAtmtBinding<decltype(bindingID)>
    {
        layoutBuilder.addInputAttachment(stageFlags, bindingID);
        poolBuilder.addInputAttachmentCount(1);
        InputAttachmentInfo info                             = {imageView};
        inputAttachments[(vulk::cpp2::GBufBinding)bindingID] = info;
        return *this;
    }

    std::shared_ptr<const VulkDescriptorSetInfo> build() {
        std::shared_ptr<const VulkDescriptorSetLayout> descriptorSetLayout;
        if (this->descriptorSetLayoutOverride) {
            descriptorSetLayout = this->descriptorSetLayoutOverride;
        } else {
            descriptorSetLayout = layoutBuilder.build();
        }
        VkDescriptorPool pool = poolBuilder.build(MAX_FRAMES_IN_FLIGHT);
        std::array<std::shared_ptr<const VulkDescriptorSet>, MAX_FRAMES_IN_FLIGHT> descriptorSets;
        for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
            auto ds = std::make_shared<VulkDescriptorSet>(vk, descriptorSetLayout->layout, pool);
            VulkDescriptorSetUpdater updater(ds);

            for (auto& pair : perFrameInfos[i].uniformSetInfos) {
                updater.addUniformBuffer(pair.second.buf, pair.second.range, pair.first);
            }
            for (auto& pair : perFrameInfos[i].ssboSetInfos) {
                updater.addStorageBuffer(pair.second.buf, pair.second.range, pair.first);
            }
            for (auto& pair : perFrameSamplerSetInfos[i]) {
                updater.addImageSampler(pair.second.imageView, pair.second.sampler, pair.first);
            }
            for (auto& [binding, info] : inputAttachments) {
                updater.addInputAttachment(info.imageView, binding);
            }

            updater.update(vk.device);
            descriptorSets[i] = ds;
        }
        return std::make_shared<const VulkDescriptorSetInfo>(vk, descriptorSetLayout, pool, std::move(descriptorSets));
    }
};