#pragma once

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#define GLM_FORCE_EXPLICIT_CTOR
#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp> // value_ptr
#include <glm/gtx/euler_angles.hpp>
#include <glm/gtx/hash.hpp>

// Coordinate System
constexpr bool IS_RIGHT_HANDED = true;

// Winding Order
constexpr VkFrontFace DEFAULT_FRONT_FACE_WINDING_ORDER = VK_FRONT_FACE_CLOCKWISE;

// Vulkan uses a right hand coordinate system with positive z pointing away from the viewer
constexpr glm::vec3 VIEWSPACE_FORWARD_VEC = glm::vec3(0.0f, 0.0f, 1.0f);
constexpr glm::vec3 VIEWSPACE_UP_VEC = glm::vec3(0.0f, -1.0f, 0.0f);
constexpr glm::vec3 VIEWSPACE_RIGHT_VEC = glm::vec3(1.0f, 0.0f, 0.0f);

// Camera Settings
constexpr float DEFAULT_FOV_RADS = glm::radians(45.0f);
// constexpr float ASPECT_RATIO = 16.0f / 9.0f;
// constexpr float NEAR_PLANE = 0.1f;
// constexpr float FAR_PLANE = 100.0f;
