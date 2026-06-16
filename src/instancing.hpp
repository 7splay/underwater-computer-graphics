#pragma once

#include <glm/glm.hpp>

#include "models.hpp"

// Lightweight instance: shares GPU buffers/textures from cached base mesh.
inline Renderable cloneRenderable(const Renderable &source,
                                  const glm::mat4 &modelMatrix) {
  Renderable instance = source;
  instance.model = modelMatrix;
  instance.instanceCount = 1;
  instance.instVBO = 0;
  instance.instanceMatrices.clear();
  return instance;
}
