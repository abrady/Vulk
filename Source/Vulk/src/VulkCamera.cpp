#include "Vulk/VulkCamera.h"

static glm::quat lookAtToQuaternion(const glm::vec3& eye, const glm::vec3& center, const glm::vec3& up) {
    // Create the lookAt matrix
    glm::mat4 viewMatrix = glm::lookAt(eye, center, up);

    // Extract the rotation part of the view matrix
    glm::mat3 rotationMatrix(viewMatrix);

    // Convert the rotation matrix to a quaternion
    glm::quat orientation = glm::quat_cast(rotationMatrix);

    return orientation;
}

VulkCamera::VulkCamera() {
    setLookAt(eye, lookAt);
}

void VulkCamera::setLookAt(glm::vec3 eyeIn, glm::vec3 target) {
    this->eye = eyeIn;
    this->lookAt = target;
    this->orientation = lookAtToQuaternion(eye, target, this->up);
}

glm::mat3 VulkCamera::getRotMat() {
    return glm::mat3_cast(orientation);
}
glm::vec3 VulkCamera::getForwardVec() {
    return glm::normalize(orientation * DEFAULT_FORWARD_VEC);
}
glm::vec3 VulkCamera::getRightVec() {
    glm::vec3 right = glm::vec3(1.0f, 0.0f, 0.0f);
    return getRotMat() * right;
}
glm::vec3 VulkCamera::getUpVec() {
    return getRotMat() * DEFAULT_UP_VEC;
}