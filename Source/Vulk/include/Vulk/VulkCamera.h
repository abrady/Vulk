#pragma once

#include "VulkUtil.h"

struct VulkCamera {
    glm::vec3 eye = glm::vec3(0.4f, .85f, 2.4f);
    glm::vec3 lookAt = glm::vec3(0.0f, 0.0f, 0.0f);
    float yaw = 180.0f;
    float pitch = -15.0f;
    float nearClip = 0.01f;
    float farClip = 100.0f;

    void setLookAt(glm::vec3 eyeIn, glm::vec3 target);
    glm::mat3 getRotMat();
    glm::vec3 getForwardVec();
    glm::vec3 getRightVec();
    glm::vec3 getUpVec();
};