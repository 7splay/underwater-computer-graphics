#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/constants.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>

#include <algorithm>
#include <cmath>

#include "seabed_height.hpp"

namespace {
// Playable bounds — sand extends further (see scene.cpp) so the mesh edge stays
// out of sight while the camera still hits an invisible wall here.
constexpr float kCameraHalfExtentX = 50.0f;
constexpr float kCameraHalfExtentZ = 50.0f;
constexpr float kCameraMinAboveSeabed = 1.15f;
constexpr float kCameraMaxHeight = 14.0f;
}  // namespace

inline glm::quat cameraOrientation = glm::quat(1.0f, 0.0f, 0.0f, 0.0f);
inline glm::vec3 cameraPosition = glm::vec3(0.0f, 1.5f, 6.0f);

inline float yawAngle = 0.0f;
inline float pitchAngle = 0.0f;
inline float pitchLimit = glm::radians(89.0f);

inline glm::vec3 cameraVelocity = glm::vec3(0.0f);
inline float movementSmoothness = 2.0f;

inline void syncCameraOrientation() {
  const auto rotationX =
      glm::quat(std::cos(pitchAngle / 2.0f), std::sin(pitchAngle / 2.0f), 0.0f,
                0.0f);
  const auto rotationY =
      glm::quat(std::cos(yawAngle / 2.0f), 0.0f, std::sin(yawAngle / 2.0f),
                0.0f);
  cameraOrientation = glm::normalize(rotationY * rotationX);
}

inline void initCamera() {
  cameraPosition = glm::vec3(-9.0f, 1.4f, -6.0f);
  yawAngle = 0.52f;
  pitchAngle = -0.18f;
  cameraVelocity = glm::vec3(0.0f);
  syncCameraOrientation();
}

inline glm::mat4 getViewMatrix() {
  return glm::mat4_cast(glm::conjugate(cameraOrientation)) *
         glm::translate(glm::mat4(1.0f), -cameraPosition);
}

inline void rotateCamera(float yaw, float pitch, float amount) {
  yawAngle += yaw * amount;
  yawAngle = glm::mod(yawAngle, glm::two_pi<float>());
  pitchAngle += pitch * amount;
  pitchAngle = glm::clamp(pitchAngle, -pitchLimit, pitchLimit);

  syncCameraOrientation();
}

inline void updateCameraMovement(glm::vec3 inputDirection,
                                 glm::vec3 independentInputDirection,
                                 float speed, float deltaTime) {
  glm::vec3 desiredDirection =
      cameraOrientation * inputDirection + independentInputDirection;

  if (glm::length(desiredDirection) > 0.0f) {
    desiredDirection = glm::normalize(desiredDirection);
  }

  glm::vec3 desiredVelocity = desiredDirection * speed;
  float alpha = 1.0f - std::exp(-movementSmoothness * deltaTime);
  cameraVelocity = glm::mix(cameraVelocity, desiredVelocity, alpha);
  cameraPosition += cameraVelocity * deltaTime;
}

inline void clampCameraToScene(const SeabedParams &seabed = kSceneSeabed) {
  if (cameraPosition.x < -kCameraHalfExtentX) {
    cameraPosition.x = -kCameraHalfExtentX;
    if (cameraVelocity.x < 0.0f) {
      cameraVelocity.x = 0.0f;
    }
  } else if (cameraPosition.x > kCameraHalfExtentX) {
    cameraPosition.x = kCameraHalfExtentX;
    if (cameraVelocity.x > 0.0f) {
      cameraVelocity.x = 0.0f;
    }
  }

  if (cameraPosition.z < -kCameraHalfExtentZ) {
    cameraPosition.z = -kCameraHalfExtentZ;
    if (cameraVelocity.z < 0.0f) {
      cameraVelocity.z = 0.0f;
    }
  } else if (cameraPosition.z > kCameraHalfExtentZ) {
    cameraPosition.z = kCameraHalfExtentZ;
    if (cameraVelocity.z > 0.0f) {
      cameraVelocity.z = 0.0f;
    }
  }

  const float seabedY =
      sampleSeabedWorldHeight(cameraPosition.x, cameraPosition.z, seabed);
  const float minY = seabedY + kCameraMinAboveSeabed;
  if (cameraPosition.y < minY) {
    cameraPosition.y = minY;
    if (cameraVelocity.y < 0.0f) {
      cameraVelocity.y = 0.0f;
    }
  }
  if (cameraPosition.y > kCameraMaxHeight) {
    cameraPosition.y = kCameraMaxHeight;
    if (cameraVelocity.y > 0.0f) {
      cameraVelocity.y = 0.0f;
    }
  }
}
