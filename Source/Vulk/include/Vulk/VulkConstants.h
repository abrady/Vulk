#pragma once

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#define GLM_FORCE_EXPLICIT_CTOR
#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/euler_angles.hpp>
#include <glm/gtx/hash.hpp>

// Coordinate System
constexpr bool IS_RIGHT_HANDED = true;

// Winding Order
constexpr VkFrontFace FRONT_FACE = VK_FRONT_FACE_COUNTER_CLOCKWISE; // VK_FRONT_FACE_COUNTER_CLOCKWISE for counter-clockwise, VK_FRONT_FACE_CLOCKWISE for clockwise

// Default Vectors
constexpr glm::vec3 DEFAULT_FORWARD_VEC = glm::vec3(0.0f, 0.0f, -1.0f);
constexpr glm::vec3 DEFAULT_UP_VEC = glm::vec3(0.0f, 1.0f, 0.0f);
constexpr glm::vec3 DEFAULT_RIGHT_VEC = glm::vec3(1.0f, 0.0f, 0.0f);

// Camera Settings
// constexpr float FOV = 45.0f; // In degrees
// constexpr float ASPECT_RATIO = 16.0f / 9.0f;
// constexpr float NEAR_PLANE = 0.1f;
// constexpr float FAR_PLANE = 100.0f;
