#pragma once
#include <memory>

#include "VulkImageView.h"

struct VulkMaterialTextures {
    std::shared_ptr<VulkTextureView> diffuseView;
    std::shared_ptr<VulkTextureView> normalView;
    std::shared_ptr<VulkTextureView> ambientOcclusionView;
    std::shared_ptr<VulkTextureView> displacementView;
    std::shared_ptr<VulkTextureView> metallicView;
    std::shared_ptr<VulkTextureView> roughnessView;
};
