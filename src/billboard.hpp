#pragma once

#include "instancing.hpp"
#include "vertex_layout.hpp"

#include <GL/glew.h>

#include <vector>

inline Renderable createBillboardRenderable(const Renderable &source,
                                            float width, float height) {
  const float halfW = width * 0.5f;
  const float halfH = height * 0.5f;

  const std::vector<ModelVertex> vertices = {
      {{-halfW, -halfH, 0.0f}, {0.0f, 0.0f, 1.0f}, {0.0f, 0.0f}, {1, 0, 0, 1}},
      {{halfW, -halfH, 0.0f}, {0.0f, 0.0f, 1.0f}, {1.0f, 0.0f}, {1, 0, 0, 1}},
      {{halfW, halfH, 0.0f}, {0.0f, 0.0f, 1.0f}, {1.0f, 1.0f}, {1, 0, 0, 1}},
      {{-halfW, halfH, 0.0f}, {0.0f, 0.0f, 1.0f}, {0.0f, 1.0f}, {1, 0, 0, 1}},
  };
  const std::vector<unsigned int> indices = {0, 1, 2, 2, 3, 0};

  Renderable billboard{};
  billboard.texture = source.texture;
  billboard.normalTexture = source.normalTexture;
  billboard.roughnessTexture = source.roughnessTexture;
  billboard.metallicTexture = source.metallicTexture;
  billboard.useNormalMap = false;
  billboard.useRoughnessMap = source.useRoughnessMap;
  billboard.useMetallicMap = source.useMetallicMap;
  billboard.useArmMap = source.useArmMap;
  billboard.normalStrength = source.normalStrength;
  billboard.metallic = source.metallic;
  billboard.roughness = source.roughness;
  billboard.albedoTint = source.albedoTint;
  billboard.opacityTexture = source.opacityTexture;
  billboard.useAlphaCutout = source.useAlphaCutout;
  billboard.useOpacityCutout = source.useOpacityCutout;
  billboard.alphaCutoff = source.alphaCutoff;
  billboard.isDoubleSided = source.isDoubleSided;
  billboard.indexCount = static_cast<GLsizei>(indices.size());
  billboard.isBillboard = true;

  glGenVertexArrays(1, &billboard.VAO);
  glGenBuffers(1, &billboard.VBO);
  glGenBuffers(1, &billboard.EBO);

  glBindVertexArray(billboard.VAO);
  glBindBuffer(GL_ARRAY_BUFFER, billboard.VBO);
  glBufferData(GL_ARRAY_BUFFER,
               static_cast<GLsizeiptr>(vertices.size() * sizeof(ModelVertex)),
               vertices.data(), GL_STATIC_DRAW);
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, billboard.EBO);
  glBufferData(GL_ELEMENT_ARRAY_BUFFER,
               static_cast<GLsizeiptr>(indices.size() * sizeof(unsigned int)),
               indices.data(), GL_STATIC_DRAW);
  bindModelVertexLayout();
  glBindVertexArray(0);

  return billboard;
}

inline glm::mat4 billboardInstanceMatrix(const glm::mat4 &sourceTransform,
                                           float widthScale,
                                           float heightScale) {
  const glm::vec3 position = glm::vec3(sourceTransform[3]);
  glm::mat4 billboard = glm::translate(glm::mat4(1.0f), position);
  billboard = glm::scale(billboard, glm::vec3(widthScale, heightScale, 1.0f));
  return billboard;
}

inline float extractUniformScale(const glm::mat4 &transform) {
  const float sx = glm::length(glm::vec3(transform[0]));
  const float sy = glm::length(glm::vec3(transform[1]));
  const float sz = glm::length(glm::vec3(transform[2]));
  return (sx + sy + sz) / 3.0f;
}
