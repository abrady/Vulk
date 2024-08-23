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
    this->eye         = eyeIn;
    this->lookAt      = target;
    this->orientation = lookAtToQuaternion(eye, target, this->up);
}

glm::mat4 VulkCamera::getViewMat() {
    glm::mat4 trans = glm::translate(glm::mat4(1.0f), -eye);
    glm::mat4 rot   = glm::mat4_cast(orientation);
    return rot * trans;
}

glm::vec3 VulkCamera::getForwardVec() {
    glm::quat invOrientation = glm::inverse(orientation);
    return glm::normalize(invOrientation * VIEWSPACE_FORWARD_VEC);
}
glm::vec3 VulkCamera::getRightVec() {
    glm::quat invOrientation = glm::inverse(orientation);
    return glm::mat3_cast(invOrientation) * VIEWSPACE_RIGHT_VEC;
}
glm::vec3 VulkCamera::getUpVec() {
    glm::quat invOrientation = glm::inverse(orientation);
    return glm::mat3_cast(invOrientation) * VIEWSPACE_UP_VEC;
}

glm::vec3 VulkCamera::getEulers() {
    return glm::eulerAngles(orientation);
}

void VulkCamera::updateOrientation(float dx, float dy) {
    glm::quat pitch = glm::angleAxis(dy, VIEWSPACE_RIGHT_VEC);
    glm::quat yaw   = glm::angleAxis(-dx, VIEWSPACE_UP_VEC);
    orientation     = glm::normalize(pitch * yaw * orientation);
}

void VulkCamera::updatePosition(float dx, float dy, float dz) {
    glm::vec3 forward = getForwardVec();
    glm::vec3 right   = getRightVec();
    glm::vec3 upv     = getUpVec();

    eye += right * dx + upv * dy + forward * dz;
}
