#pragma once

#include "VulkUtil.h"

class VulkCamera {
    // ============================
    // only for initialization.
    // TODO: break this out into the metadata and just convert to a quaternion
    // on load
    glm::vec3 lookAt = glm::vec3(0.0f, 0.0f, 0.0f);
    glm::vec3 up     = glm::vec3(0.0f, 1.0f, 0.0f);
    // ============================
   public:
    glm::vec3 eye         = glm::vec3(0.4f, .85f, 2.4f);
    glm::quat orientation = glm::quat(1.0f, 0.0f, 0.0f, 0.0f);
    float nearClip        = 1.f;
    float farClip         = 100.0f;

    VulkCamera();

    void setLookAt(glm::vec3 eyeIn, glm::vec3 target);
    glm::mat4 getViewMat() const;
    glm::vec3 getForwardVec() const;
    glm::vec3 getRightVec() const;
    glm::vec3 getUpVec() const;
    glm::vec3 getEulers() const;

    void updateOrientation(float dx, float dy);
    void updatePosition(float dx, float dy, float dz);
};