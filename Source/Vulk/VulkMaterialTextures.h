#pragma once
#include <memory>

#include "VulkTextureView.h"

struct VulkMaterialTextures
{
    std::shared_ptr<VulkTextureView> diffuseView;
    std::shared_ptr<VulkTextureView> normalView;
    // could also have ambient and specular but not using them for now
};
