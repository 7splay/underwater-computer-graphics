#pragma once

#include <cstddef>

#include "renderable.hpp"

#include <GL/glew.h>

// Shared vertex layout for seabed and imported meshes (TBN for normal mapping).
inline void bindModelVertexLayout() {
  glVertexAttribPointer(
      0, 3, GL_FLOAT, GL_FALSE, sizeof(ModelVertex),
      reinterpret_cast<void *>(offsetof(ModelVertex, position)));
  glEnableVertexAttribArray(0);

  glVertexAttribPointer(
      1, 3, GL_FLOAT, GL_FALSE, sizeof(ModelVertex),
      reinterpret_cast<void *>(offsetof(ModelVertex, normal)));
  glEnableVertexAttribArray(1);

  glVertexAttribPointer(
      2, 2, GL_FLOAT, GL_FALSE, sizeof(ModelVertex),
      reinterpret_cast<void *>(offsetof(ModelVertex, texCoord)));
  glEnableVertexAttribArray(2);

  glVertexAttribPointer(
      3, 4, GL_FLOAT, GL_FALSE, sizeof(ModelVertex),
      reinterpret_cast<void *>(offsetof(ModelVertex, tangent)));
  glEnableVertexAttribArray(3);
}
