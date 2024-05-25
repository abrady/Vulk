#pragma once

#include <cereal/archives/binary.hpp>
#include <cereal/archives/json.hpp>
#include <cereal/types/memory.hpp>

#include "VulkUtil.h"

class VulkCamera {
    // ============================
    // only for initialization.
    // TODO: break this out into the metadata and just convert to a quaternion
    // on load
    glm::vec3 eye = glm::vec3(0.4f, .85f, 2.4f);
    glm::vec3 lookAt = glm::vec3(0.0f, 0.0f, 0.0f);
    glm::vec3 up = glm::vec3(0.0f, 1.0f, 0.0f);
    // ============================
public:
    glm::quat orientation = glm::quat(1.0f, 0.0f, 0.0f, 0.0f);
    float nearClip = 1.f;
    float farClip = 100.0f;

    VulkCamera();

    void setLookAt(glm::vec3 eyeIn, glm::vec3 target);
    glm::mat3 getRotMat();
    glm::vec3 getForwardVec();
    glm::vec3 getRightVec();
    glm::vec3 getUpVec();

    template <class Archive>
    void serialize(Archive& archive) {
        archive(CEREAL_NVP(eye), CEREAL_NVP(lookAt), CEREAL_NVP(orientation));
    }
};