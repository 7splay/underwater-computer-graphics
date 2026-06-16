#pragma once

#include "PerlinNoise.hpp"
#include "materials.hpp"
#include "seabed_height.hpp"
#include "vertex_layout.hpp"

#include <algorithm>
#include <cmath>
#include <vector>

// grid[z][x] in [0, 1], dimensions (resZ+1) x (resX+1)
// higher smoothness -> lower noise frequency -> wider, gentler dunes
inline std::vector<std::vector<float>> generateHeightMap(int resX, int resZ,
                                                         float smoothness) {
  std::vector<std::vector<float>> grid(resZ + 1, std::vector<float>(resX + 1));
  for (int z = 0; z <= resZ; ++z) {
    for (int x = 0; x <= resX; ++x) {
      grid[z][x] =
          seabedNoise.octave2D_01(x / smoothness, z / smoothness, 3);
    }
  }
  return grid;
}

inline Renderable generateSand(int resX, int resZ, float gap, float amplitude,
                               float smoothness,
                               glm::mat4 model = glm::mat4(1.0f)) {
  const float uvScale = 10.0f;
  std::vector<std::vector<float>> heightMap = generateHeightMap(resX, resZ, smoothness);

  auto sampleHeight = [&](int sx, int sz) -> float {
    sx = std::clamp(sx, 0, resX);
    sz = std::clamp(sz, 0, resZ);
    return heightMap[static_cast<std::size_t>(sz)][static_cast<std::size_t>(sx)] *
               amplitude -
           amplitude * 0.5f;
  };

  std::vector<ModelVertex> vertices;
  vertices.reserve(static_cast<std::size_t>((resX + 1) * (resZ + 1)));
  for (int z = 0; z <= resZ; ++z) {
    for (int x = 0; x <= resX; ++x) {
      ModelVertex vertex{};

      vertex.position[0] = (x - resX * 0.5f) * gap;
      vertex.position[1] = sampleHeight(x, z);
      vertex.position[2] = (z - resZ * 0.5f) * gap;

      vertex.texCoord[0] =
          static_cast<float>(x) / static_cast<float>(resX) * uvScale;
      vertex.texCoord[1] =
          static_cast<float>(z) / static_cast<float>(resZ) * uvScale;

      const float dx = 2.0f * gap;
      const float dz = 2.0f * gap;
      const float slopeX = (sampleHeight(x + 1, z) - sampleHeight(x - 1, z)) / dx;
      const float slopeZ = (sampleHeight(x, z + 1) - sampleHeight(x, z - 1)) / dz;

      const glm::vec3 normal = glm::normalize(glm::vec3(-slopeX, 1.0f, -slopeZ));
      vertex.normal[0] = normal.x;
      vertex.normal[1] = normal.y;
      vertex.normal[2] = normal.z;

      // Tangent frame for sand normal mapping in model.frag.
      glm::vec3 tangent = glm::normalize(glm::vec3(1.0f, slopeX, 0.0f));
      tangent = glm::normalize(tangent - glm::dot(tangent, normal) * normal);
      const glm::vec3 bitangent =
          glm::normalize(glm::vec3(0.0f, slopeZ, 1.0f));
      const float handedness =
          glm::dot(glm::cross(normal, tangent), bitangent) < 0.0f ? -1.0f : 1.0f;

      vertex.tangent[0] = tangent.x;
      vertex.tangent[1] = tangent.y;
      vertex.tangent[2] = tangent.z;
      vertex.tangent[3] = handedness;

      vertices.push_back(vertex);
    }
  }

  std::vector<unsigned int> indices;
  indices.reserve(static_cast<std::size_t>(resX * resZ * 6));
  for (int z = 0; z < resZ; ++z) {
    for (int x = 0; x < resX; ++x) {
      const unsigned int topLeft =
          static_cast<unsigned int>(z * (resX + 1) + x);
      const unsigned int topRight = topLeft + 1u;
      const unsigned int bottomLeft =
          static_cast<unsigned int>((z + 1) * (resX + 1) + x);
      const unsigned int bottomRight = bottomLeft + 1u;

      indices.push_back(topLeft);
      indices.push_back(bottomLeft);
      indices.push_back(topRight);

      indices.push_back(topRight);
      indices.push_back(bottomLeft);
      indices.push_back(bottomRight);
    }
  }

  Renderable sand{};
  glGenVertexArrays(1, &sand.VAO);
  glGenBuffers(1, &sand.VBO);
  glGenBuffers(1, &sand.EBO);

  glBindVertexArray(sand.VAO);

  glBindBuffer(GL_ARRAY_BUFFER, sand.VBO);
  glBufferData(GL_ARRAY_BUFFER,
               static_cast<GLsizeiptr>(vertices.size() * sizeof(ModelVertex)),
               vertices.data(), GL_STATIC_DRAW);

  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, sand.EBO);
  glBufferData(GL_ELEMENT_ARRAY_BUFFER,
               static_cast<GLsizeiptr>(indices.size() * sizeof(unsigned int)),
               indices.data(), GL_STATIC_DRAW);

  bindModelVertexLayout();
  glBindVertexArray(0);

  sand.indexCount = static_cast<GLsizei>(indices.size());
  sand.model = model;
  loadSandTextures(sand);
  return sand;
}