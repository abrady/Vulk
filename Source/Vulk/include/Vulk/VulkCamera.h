#pragma once

#include "VulkUtil.h"
#include <cereal/archives/binary.hpp>
#include <cereal/archives/json.hpp>
#include <cereal/types/memory.hpp>

struct VulkCamera {
    glm::vec3 eye = glm::vec3(0.4f, .85f, 2.4f);
    glm::vec3 lookAt = glm::vec3(0.0f, 0.0f, 0.0f);
    float yaw = 180.0f;
    float pitch = -15.0f;
    float nearClip = 1.f;
    float farClip = 100.0f;

    void setLookAt(glm::vec3 eyeIn, glm::vec3 target);
    glm::mat3 getRotMat();
    glm::vec3 getForwardVec();
    glm::vec3 getRightVec();
    glm::vec3 getUpVec();

    template <class Archive> void serialize(Archive &archive) {
        archive(CEREAL_NVP(eye), CEREAL_NVP(lookAt), CEREAL_NVP(yaw), CEREAL_NVP(pitch), CEREAL_NVP(nearClip), CEREAL_NVP(farClip));
    }
};