#pragma once

#include "camera.hpp"

#include <cmath>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>

namespace flashlight {

inline bool enabled = true;

constexpr float kConeHalfAngleDeg = 21.0f;  // 42 total cone
constexpr float kRange = 50.0f;
// near plane pushed well out: with the old tiny near, perspective depth was so
// nonlinear that everything 15-35m away mapped to 0.94-1.0, so a coral and the
// sand behind it differed by less than the shadow bias and cast shadows
// vanished. a larger near spreads the usable depth range across the distances
// we actually care about (anything closer just won't cast)
constexpr float kNear = 2.0f;
constexpr float kFar = 45.0f;
// shadow frustum is wider than the lit cone so casters at the beam edge still
// rasterise fully into the depth map, avoiding cropped shadows
constexpr float kShadowFrustumExtraDeg = 6.0f;
// helmet light: well back and high above the camera, aimed far and slightly
// down so the beam tilts onto the seabed in front of the diver
constexpr float kHelmetBack = -4.0f;
constexpr float kHelmetUp = 2.0f;
constexpr float kTargetForward = 15.0f;
constexpr float kTargetBelow = 1.0f;

inline glm::vec3 forward() {
  return glm::normalize(cameraOrientation * glm::vec3(0.0f, 0.0f, -1.0f));
}
inline glm::vec3 up() {
  return glm::normalize(cameraOrientation * glm::vec3(0.0f, 1.0f, 0.0f));
}

inline glm::vec3 position() {
  return cameraPosition + forward() * kHelmetBack + up() * kHelmetUp;
}

inline glm::vec3 direction() {
  const glm::vec3 target = cameraPosition + forward() * kTargetForward -
                           glm::vec3(0.0f, kTargetBelow, 0.0f);
  return glm::normalize(target - position());
}

inline glm::mat4 viewMatrix() {
  // follow the camera up so the beam doesn't drift sideways on look-around
  return glm::lookAt(position(), position() + direction(), up());
}

inline glm::mat4 projectionMatrix(float aspect = 1.0f) {
  const float frustumHalfDeg = kConeHalfAngleDeg + kShadowFrustumExtraDeg;
  return glm::perspective(glm::radians(frustumHalfDeg * 2.0f), aspect, kNear,
                          kFar);
}

// shadow-pass transform (no texture bias)
inline glm::mat4 viewProjectionMatrix(float aspect = 1.0f) {
  return projectionMatrix(aspect) * viewMatrix();
}

// biased transform for shadow sampling in the lit pass
inline glm::mat4 biasedLightSpaceMatrix(float aspect = 1.0f) {
  const glm::mat4 bias(0.5f, 0.0f, 0.0f, 0.0f, 0.0f, 0.5f, 0.0f, 0.0f, 0.0f,
                       0.0f, 0.5f, 0.0f, 0.5f, 0.5f, 0.5f, 1.0f);
  return bias * viewProjectionMatrix(aspect);
}

inline bool hitsPoint(const glm::vec3 &p, float scareRadius) {
  if (!enabled) {
    return false;
  }
  const glm::vec3 toP = p - position();
  const float dist = glm::length(toP);
  // inside the cone AND within the user-defined scare radius, which can be
  // much tighter than the full visible beam so fish only react when the
  // light is really on them
  if (dist > scareRadius || dist < 0.001f) {
    return false;
  }
  const float cosA = glm::dot(toP / dist, direction());
  return cosA > std::cos(glm::radians(kConeHalfAngleDeg));
}

}  // namespace flashlight
