#pragma once

#include <GL/glew.h>

#include <glm/glm.hpp>
#include <vector>

struct Renderable {
  GLuint VAO = 0;
  GLuint VBO = 0;
  GLuint EBO = 0;
  GLuint texture = 0;
  GLuint normalTexture = 0;
  GLuint roughnessTexture = 0;
  GLuint metallicTexture = 0;
  bool useNormalMap = false;
  bool useRoughnessMap = false;
  bool useMetallicMap = false;
  bool useArmMap = false;
  // cutout-style meshes (fish fins, seaweed) discard low-alpha fragments
  bool useAlphaTest = false;
  float alphaCutoff = 0.5f;
  float normalStrength = 1.0f;
  float metallic = 0.0f;
  float roughness = 0.5f;
  glm::vec3 albedoTint = glm::vec3(1.0f);
  GLsizei indexCount = 0;
  GLsizei instanceCount = 1;
  GLuint instVBO = 0;
  glm::mat4 model = glm::mat4(1.0f);
  std::vector<glm::mat4> instanceMatrices;
};

struct ModelVertex {
  float position[3];
  float normal[3];
  float texCoord[2];
  float tangent[4];
};
