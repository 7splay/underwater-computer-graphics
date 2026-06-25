#pragma once

#include <GL/glew.h>

#include <glm/glm.hpp>

#include <cstddef>
#include <vector>

#include "models.hpp"
#include "vertex_layout.hpp"

inline void bindInstancedAttributes(GLuint instVBO) {
  glBindBuffer(GL_ARRAY_BUFFER, instVBO);
  const std::size_t vec4Size = sizeof(glm::vec4);
  for (int row = 0; row < 4; ++row) {
    const GLuint location = static_cast<GLuint>(4 + row);
    glEnableVertexAttribArray(location);
    glVertexAttribPointer(location, 4, GL_FLOAT, GL_FALSE, sizeof(glm::mat4),
                         reinterpret_cast<void *>(row * vec4Size));
    glVertexAttribDivisor(location, 1);
  }
}

inline void clearInstancing(Renderable &renderable) {
  renderable.instanceCount = 0;
  renderable.instanceMatrices.clear();
}

inline void uploadInstanceMatrices(Renderable &renderable,
                                   const std::vector<glm::mat4> &matrices) {
  const GLsizei previousCount = renderable.instanceCount;
  const GLsizei newCount = static_cast<GLsizei>(matrices.size());
  renderable.instanceMatrices = matrices;
  renderable.instanceCount = newCount;

  if (newCount <= 0) {
    return;
  }

  const bool firstUpload = renderable.instVBO == 0;
  if (firstUpload) {
    glGenBuffers(1, &renderable.instVBO);
  }

  glBindVertexArray(renderable.VAO);
  glBindBuffer(GL_ARRAY_BUFFER, renderable.instVBO);
  const GLsizeiptr byteSize =
      static_cast<GLsizeiptr>(matrices.size() * sizeof(glm::mat4));
  if (!firstUpload && newCount == previousCount) {
    glBufferSubData(GL_ARRAY_BUFFER, 0, byteSize, matrices.data());
  } else {
    glBufferData(GL_ARRAY_BUFFER, byteSize, matrices.data(), GL_DYNAMIC_DRAW);
    bindInstancedAttributes(renderable.instVBO);
  }
  glBindVertexArray(0);
}

inline Renderable cloneRenderable(const Renderable &source,
                                  const glm::mat4 &modelMatrix) {
  Renderable instance = source;
  instance.model = modelMatrix;
  instance.instanceCount = 1;
  instance.instVBO = 0;
  instance.instanceMatrices.clear();
  instance.isBillboard = false;
  return instance;
}

inline Renderable shareMeshRenderable(const Renderable &source) {
  Renderable shared = source;
  shared.model = glm::mat4(1.0f);
  shared.instanceCount = 0;
  shared.instVBO = 0;
  shared.instanceMatrices.clear();
  shared.isBillboard = false;
  return shared;
}

inline Renderable cloneMeshWithOwnVao(const Renderable &source) {
  Renderable instance = shareMeshRenderable(source);
  if (source.VAO == 0 || source.VBO == 0 || source.EBO == 0) {
    return instance;
  }

  glGenVertexArrays(1, &instance.VAO);
  glBindVertexArray(instance.VAO);
  glBindBuffer(GL_ARRAY_BUFFER, source.VBO);
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, source.EBO);
  bindModelVertexLayout();
  glBindVertexArray(0);
  return instance;
}

inline Renderable createSimplifiedMesh(const Renderable &source,
                                       int keepEveryNthTriangle = 3) {
  if (source.EBO == 0 || source.indexCount < 3 ||
      keepEveryNthTriangle <= 1) {
    return cloneMeshWithOwnVao(source);
  }

  Renderable simplified = shareMeshRenderable(source);
  simplified.useNormalMap = false;
  simplified.normalStrength = 0.0f;

  const std::size_t indexCount = static_cast<std::size_t>(source.indexCount);
  std::vector<unsigned int> indices(indexCount);
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, source.EBO);
  glGetBufferSubData(
      GL_ELEMENT_ARRAY_BUFFER, 0,
      static_cast<GLsizeiptr>(indices.size() * sizeof(unsigned int)),
      indices.data());

  std::vector<unsigned int> reduced;
  reduced.reserve(indexCount / static_cast<std::size_t>(keepEveryNthTriangle) +
                  3);
  for (std::size_t tri = 0; tri + 2 < indexCount; tri += 3) {
    if ((tri / 3) % static_cast<std::size_t>(keepEveryNthTriangle) != 0) {
      continue;
    }
    reduced.push_back(indices[tri]);
    reduced.push_back(indices[tri + 1]);
    reduced.push_back(indices[tri + 2]);
  }

  if (reduced.size() < 3) {
    return cloneMeshWithOwnVao(source);
  }

  glGenVertexArrays(1, &simplified.VAO);
  glGenBuffers(1, &simplified.EBO);
  glBindVertexArray(simplified.VAO);
  glBindBuffer(GL_ARRAY_BUFFER, source.VBO);
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, simplified.EBO);
  glBufferData(GL_ELEMENT_ARRAY_BUFFER,
               static_cast<GLsizeiptr>(reduced.size() * sizeof(unsigned int)),
               reduced.data(), GL_STATIC_DRAW);
  bindModelVertexLayout();
  glBindVertexArray(0);
  simplified.indexCount = static_cast<GLsizei>(reduced.size());
  return simplified;
}

inline void bindRenderableInstancing(const Renderable &renderable) {
  if (renderable.instVBO == 0 || renderable.instanceCount <= 0) {
    return;
  }
  glBindVertexArray(renderable.VAO);
  bindInstancedAttributes(renderable.instVBO);
}
