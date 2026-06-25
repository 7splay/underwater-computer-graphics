#pragma once

#include "textures.hpp"
#include "vertex_layout.hpp"

#include <filesystem>

inline Renderable createClusterSeaweedCrossMesh() {
  constexpr float halfWidth = 1.05f;
  constexpr float height = 2.8f;

  const std::vector<ModelVertex> vertices = {
      {{-halfWidth, 0.0f, 0.0f}, {0.0f, 0.0f, 1.0f}, {0.0f, 0.0f}, {1, 0, 0, 1}},
      {{halfWidth, 0.0f, 0.0f}, {0.0f, 0.0f, 1.0f}, {1.0f, 0.0f}, {1, 0, 0, 1}},
      {{halfWidth, height, 0.0f}, {0.0f, 0.0f, 1.0f}, {1.0f, 1.0f}, {1, 0, 0, 1}},
      {{-halfWidth, height, 0.0f}, {0.0f, 0.0f, 1.0f}, {0.0f, 1.0f}, {1, 0, 0, 1}},

      {{0.0f, 0.0f, -halfWidth}, {1.0f, 0.0f, 0.0f}, {0.0f, 0.0f}, {0, 0, 1, 1}},
      {{0.0f, 0.0f, halfWidth}, {1.0f, 0.0f, 0.0f}, {1.0f, 0.0f}, {0, 0, 1, 1}},
      {{0.0f, height, halfWidth}, {1.0f, 0.0f, 0.0f}, {1.0f, 1.0f}, {0, 0, 1, 1}},
      {{0.0f, height, -halfWidth}, {1.0f, 0.0f, 0.0f}, {0.0f, 1.0f}, {0, 0, 1, 1}},
  };
  const std::vector<unsigned int> indices = {
      0, 1, 2, 2, 3, 0, 4, 5, 6, 6, 7, 4,
  };

  Renderable seaweed{};
  seaweed.indexCount = static_cast<GLsizei>(indices.size());
  seaweed.metallic = 0.0f;
  seaweed.roughness = 0.78f;
  seaweed.albedoTint = glm::vec3(1.0f, 1.0f, 1.0f);
  seaweed.useAlphaCutout = true;
  seaweed.useOpacityCutout = true;
  seaweed.alphaCutoff = 0.42f;
  seaweed.isDoubleSided = true;

  glGenVertexArrays(1, &seaweed.VAO);
  glGenBuffers(1, &seaweed.VBO);
  glGenBuffers(1, &seaweed.EBO);

  glBindVertexArray(seaweed.VAO);
  glBindBuffer(GL_ARRAY_BUFFER, seaweed.VBO);
  glBufferData(GL_ARRAY_BUFFER,
               static_cast<GLsizeiptr>(vertices.size() * sizeof(ModelVertex)),
               vertices.data(), GL_STATIC_DRAW);
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, seaweed.EBO);
  glBufferData(GL_ELEMENT_ARRAY_BUFFER,
               static_cast<GLsizeiptr>(indices.size() * sizeof(unsigned int)),
               indices.data(), GL_STATIC_DRAW);
  bindModelVertexLayout();
  glBindVertexArray(0);

  return seaweed;
}

inline Renderable loadClusterSeaweed() {
  namespace fs = std::filesystem;
  const fs::path colourPath = "img/seaweed/seaweed_colour.png";
  const fs::path opacityPath = "img/seaweed/seaweed_opacity.png";
  if (!fs::exists(colourPath)) {
    return {};
  }

  Renderable seaweed = createClusterSeaweedCrossMesh();
  seaweed.texture = loadTextureFromFile(colourPath);
  if (seaweed.texture == 0) {
    seaweed.texture = createFallbackTexture();
  }

  if (fs::exists(opacityPath)) {
    seaweed.opacityTexture = loadTextureFromFile(opacityPath);
  }
  if (seaweed.opacityTexture == 0) {
    seaweed.useOpacityCutout = false;
  }

  seaweed.normalTexture = createNormalMapFromDiffuseFile(colourPath, 2.8f);
  seaweed.useNormalMap = seaweed.normalTexture != 0;
  seaweed.normalStrength = 0.75f;
  return seaweed;
}
