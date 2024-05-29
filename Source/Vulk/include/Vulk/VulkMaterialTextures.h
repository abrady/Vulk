#pragma once
#include <memory>

#include "VulkImageView.h"

struct VulkMaterialTextures {
    std::shared_ptr<VulkImageView> diffuseView;
    std::shared_ptr<VulkImageView> normalView;
    std::shared_ptr<VulkImageView> ambientOcclusionView;
    std::shared_ptr<VulkImageView> displacementView;
    std::shared_ptr<VulkImageView> metallicView;
    std::shared_ptr<VulkImageView> roughnessView;
};
