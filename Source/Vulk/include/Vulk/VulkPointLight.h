#pragma once

#include "VulkUtil.h"

// keep in sync with Source\Shaders\Common\common.glsl
class VulkPointLight
{
public:
    glm::vec3 pos;
    float falloffStart = 0.0f;
    glm::vec3 color;
    float falloffEnd = 0.0f;
};