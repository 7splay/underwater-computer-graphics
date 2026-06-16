#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/constants.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>

#include <cmath>

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
