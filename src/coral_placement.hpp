#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <algorithm>
#include <cmath>
#include <cstddef>

#include "seabed_height.hpp"

namespace {
constexpr float kCoralMeshBaseY = 0.476f;
}

inline glm::mat4 coralPlacement(std::size_t index, std::size_t total) {
  static const SeabedParams seabed;

  const float goldenAngle = 2.39996323f;
  const float baseRadius = 0.55f;
  const float radiusStep = 0.75f;

  const float angle = static_cast<float>(index) * goldenAngle;
  const float radius =
      baseRadius + radiusStep * std::sqrt(static_cast<float>(index + 1));

  const float x = std::cos(angle) * radius;
  const float z = -0.95f - std::sin(angle) * radius;
  const float rotation = angle * 0.22f;
  const float scale = 1.10f + 0.06f * static_cast<float>(index % 4);

  const float seabedY = sampleSeabedWorldHeight(x, z, seabed);
  const float y = seabedY + kCoralMeshBaseY * scale + 0.03f;

  (void)total;

  glm::mat4 model = glm::mat4(1.0f);
  model = glm::translate(model, glm::vec3(x, y, z));
  model = glm::rotate(model, rotation, glm::vec3(0.0f, 1.0f, 0.0f));
  model = glm::scale(model, glm::vec3(scale));
  return model;
}

inline glm::mat4 fishPlacement(std::size_t index, std::size_t) {
  const float angle = static_cast<float>(index) * 1.93f;
  const int layer = static_cast<int>(index / 4);

  float x = std::cos(angle) * (4.0f + static_cast<float>(layer) * 2.5f);
  float z = -6.0f - static_cast<float>(layer) * 3.8f -
            std::sin(angle) * 2.8f;
  float y = -1.2f + std::sin(static_cast<float>(index) * 0.71f) * 2.2f +
            static_cast<float>(index % 3) * 0.35f;

  x = std::clamp(x, -28.0f, 28.0f);
  z = std::clamp(z, -40.0f, -2.0f);
  y = std::clamp(y, -4.5f, 2.5f);

  const float rotation = angle * 0.4f;
  const float scale = 0.55f + 0.05f * static_cast<float>(index % 5);

  glm::mat4 model(1.0f);
  model = glm::translate(model, glm::vec3(x, y, z));
  model = glm::rotate(model, rotation, glm::vec3(0.0f, 1.0f, 0.0f));
  model = glm::scale(model, glm::vec3(scale));
  return model;
}

inline glm::mat4 sunkenShipPlacement() {
  static const SeabedParams seabed;

  const float x = 0.0f;
  const float z = -22.0f;
  const float scale = 11.0f;
  const float seabedY = sampleSeabedWorldHeight(x, z, seabed);
  const float y = seabedY + 0.06f * scale;

  glm::mat4 model = glm::mat4(1.0f);
  model = glm::translate(model, glm::vec3(x, y, z));
  model = glm::rotate(model, glm::radians(-90.0f), glm::vec3(1.0f, 0.0f, 0.0f));
  model = glm::rotate(model, glm::radians(12.0f), glm::vec3(0.0f, 1.0f, 0.0f));
  model = glm::rotate(model, glm::radians(-20.0f), glm::vec3(0.0f, 0.0f, 1.0f));
  model = glm::rotate(model, glm::radians(6.0f), glm::vec3(0.0f, 1.0f, 0.0f));
  model = glm::scale(model, glm::vec3(scale));
  return model;
}

inline glm::mat4 backgroundPlacement(std::size_t index, std::size_t total) {
  const float baseZ = -4.5f;
  const float spacingX = 2.2f;

  float x =
      (static_cast<float>(index) - (static_cast<float>(total) - 1.0f) * 0.5f) *
      spacingX;
  float y = 0.0f;
  float z = baseZ - 0.7f * static_cast<float>(index);
  float rotation = 0.25f * static_cast<float>(index + 1);
  float scale = 1.0f + 0.08f * static_cast<float>(index % 3);

  glm::mat4 model = glm::mat4(1.0f);
  model = glm::translate(model, glm::vec3(x, y, z));
  model = glm::rotate(model, rotation, glm::vec3(0.0f, 1.0f, 0.0f));
  model = glm::scale(model, glm::vec3(scale));
  return model;
}
