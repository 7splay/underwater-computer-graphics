#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/constants.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>

inline glm::quat cameraOrientation = glm::quat(1.0f, 0.0f, 0.0f, 0.0f);
inline glm::vec3 cameraPosition = glm::vec3(0.0f, 1.5f, 6.0f);

inline float yawAngle = 0.0f;
inline float pitchAngle = 0.0f;
inline float pitchLimit = glm::radians(89.0f);

inline glm::mat4 getViewMatrix() {
  return glm::mat4_cast(glm::conjugate(cameraOrientation)) *
         glm::translate(glm::mat4(1.0f), -cameraPosition);
}

inline void moveCamera(glm::vec3 direction, float amount,
                       bool independent = false) {
  cameraPosition += independent ? amount * direction
                                : amount * (cameraOrientation * direction);
}

inline void rotateCamera(float yaw, float pitch, float amount) {
  yawAngle += yaw * amount;
  yawAngle = glm::mod(yawAngle, glm::two_pi<float>());
  pitchAngle += pitch * amount;
  pitchAngle = glm::clamp(pitchAngle, -pitchLimit, pitchLimit);

  auto rotationX =
      glm::quat(cos(pitchAngle / 2.0f), sin(pitchAngle / 2.0f), 0.0f, 0.0f);
  auto rotationY =
      glm::quat(cos(yawAngle / 2.0f), 0.0f, sin(yawAngle / 2.0f), 0.0f);
  cameraOrientation = glm::normalize(rotationY * rotationX);
}
