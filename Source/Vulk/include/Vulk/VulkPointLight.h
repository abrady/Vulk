#pragma once

#include "VulkUtil.h"

#include <cereal/archives/binary.hpp>
#include <cereal/archives/json.hpp>
#include <cereal/types/memory.hpp>

// keep in sync with Source\Shaders\Common\common.glsl
class VulkPointLight {
  public:
    glm::vec3 pos;
    float falloffStart = 0.0f;
    glm::vec3 color;
    float falloffEnd = 0.0f;

    template <class Archive> void serialize(Archive &archive) {
        archive(CEREAL_NVP(pos), CEREAL_NVP(falloffStart), CEREAL_NVP(color), CEREAL_NVP(falloffEnd));
    }
};