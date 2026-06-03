#pragma once

#include <GL/glew.h>

#include <assimp/Importer.hpp>
#include <assimp/postprocess.h>
#include <assimp/scene.h>

#include <algorithm>
#include <cstddef>
#include <filesystem>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <iostream>
#include <random>
#include <string>
#include <vector>

struct Renderable {
  GLuint VAO = 0;
  GLuint VBO = 0;
  GLuint EBO = 0;
  GLsizei indexCount = 0;
  glm::mat4 model = glm::mat4(1.0f);
};

struct ModelVertex {
  float position[3];
  float normal[3];
};

inline std::vector<ModelVertex> extractVertices(aiMesh *mesh) {
  std::vector<ModelVertex> meshVertices;
  meshVertices.reserve(mesh->mNumVertices);

  for (unsigned int i = 0; i < mesh->mNumVertices; ++i) {
    ModelVertex vertex{};
    vertex.position[0] = mesh->mVertices[i].x;
    vertex.position[1] = mesh->mVertices[i].y;
    vertex.position[2] = mesh->mVertices[i].z;

    if (mesh->HasNormals()) {
      vertex.normal[0] = mesh->mNormals[i].x;
      vertex.normal[1] = mesh->mNormals[i].y;
      vertex.normal[2] = mesh->mNormals[i].z;
    } else {
      vertex.normal[0] = 0.0f;
      vertex.normal[1] = 0.0f;
      vertex.normal[2] = 1.0f;
    }

    meshVertices.push_back(vertex);
  }

  return meshVertices;
}

inline std::vector<unsigned int> extractIndices(aiMesh *mesh) {
  std::vector<unsigned int> meshIndices;
  for (unsigned int i = 0; i < mesh->mNumFaces; ++i) {
    const aiFace &face = mesh->mFaces[i];
    for (unsigned int j = 0; j < face.mNumIndices; ++j) {
      meshIndices.push_back(face.mIndices[j]);
    }
  }
  return meshIndices;
}

inline bool createRenderableFromMesh(aiMesh *mesh, const glm::mat4 &modelMatrix,
                                     Renderable &renderable) {
  std::vector<ModelVertex> vertices = extractVertices(mesh);
  std::vector<unsigned int> indices = extractIndices(mesh);

  if (vertices.empty() || indices.empty()) {
    return false;
  }

  renderable.model = modelMatrix;
  renderable.indexCount = static_cast<GLsizei>(indices.size());

  glGenVertexArrays(1, &renderable.VAO);
  glGenBuffers(1, &renderable.VBO);
  glGenBuffers(1, &renderable.EBO);

  glBindVertexArray(renderable.VAO);

  glBindBuffer(GL_ARRAY_BUFFER, renderable.VBO);
  glBufferData(GL_ARRAY_BUFFER,
               static_cast<GLsizeiptr>(vertices.size() * sizeof(ModelVertex)),
               vertices.data(), GL_STATIC_DRAW);

  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, renderable.EBO);
  glBufferData(GL_ELEMENT_ARRAY_BUFFER,
               static_cast<GLsizeiptr>(indices.size() * sizeof(unsigned int)),
               indices.data(), GL_STATIC_DRAW);

  glVertexAttribPointer(
      0, 3, GL_FLOAT, GL_FALSE, sizeof(ModelVertex),
      reinterpret_cast<void *>(offsetof(ModelVertex, position)));
  glEnableVertexAttribArray(0);

  glVertexAttribPointer(
      1, 3, GL_FLOAT, GL_FALSE, sizeof(ModelVertex),
      reinterpret_cast<void *>(offsetof(ModelVertex, normal)));
  glEnableVertexAttribArray(1);

  glBindVertexArray(0);
  return true;
}

inline std::vector<Renderable> loadModelFile(const std::filesystem::path &path,
                                           const glm::mat4 &modelMatrix) {
  Assimp::Importer importer;
  const aiScene *scene = importer.ReadFile(
      path.string().c_str(),
      aiProcess_Triangulate | aiProcess_GenSmoothNormals |
          aiProcess_JoinIdenticalVertices | aiProcess_ImproveCacheLocality);

  if (scene == nullptr || (scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE) != 0 ||
      scene->mRootNode == nullptr) {
    std::cerr << "Assimp load error: " << importer.GetErrorString() << std::endl;
    return {};
  }

  if (scene->mNumMeshes == 0) {
    std::cerr << "Model has no meshes: " << path.string() << std::endl;
    return {};
  }

  std::vector<Renderable> loadedRenderables;
  loadedRenderables.reserve(scene->mNumMeshes);

  for (unsigned int meshIndex = 0; meshIndex < scene->mNumMeshes; ++meshIndex) {
    Renderable renderable;
    if (createRenderableFromMesh(scene->mMeshes[meshIndex], modelMatrix,
                                 renderable)) {
      loadedRenderables.push_back(renderable);
    }
  }

  return loadedRenderables;
}

inline bool isCoralModel(const std::filesystem::path &path) {
  std::string name = path.stem().string();
  std::transform(name.begin(), name.end(), name.begin(), [](unsigned char c) {
    return static_cast<char>(std::tolower(c));
  });
  return name.find("coral") != std::string::npos;
}

inline bool isFishModel(const std::filesystem::path &path) {
  std::string name = path.stem().string();
  std::transform(name.begin(), name.end(), name.begin(), [](unsigned char c) {
    return static_cast<char>(std::tolower(c));
  });
  return name.find("fish") != std::string::npos ||
         name.find("fishes") != std::string::npos;
}

inline glm::mat4 fishPlacement(std::size_t index, std::size_t) {
  static std::mt19937 rng(123456);
  std::uniform_real_distribution<float> dx(-1.2f, 1.2f);
  std::uniform_real_distribution<float> dy(-0.2f, 0.6f);
  std::uniform_real_distribution<float> dz(-0.5f, -3.0f);
  std::uniform_real_distribution<float> rot(-1.0f, 1.0f);
  std::uniform_real_distribution<float> sc(0.5f, 1.15f);

  float x = dx(rng);
  float y = 0.2f + dy(rng);
  float z = dz(rng) - static_cast<float>(index) * 0.15f;
  float rotation = rot(rng);
  float scale = sc(rng);

  glm::mat4 model(1.0f);
  model = glm::translate(model, glm::vec3(x, y, z));
  model = glm::rotate(model, rotation, glm::vec3(0.0f, 1.0f, 0.0f));
  model = glm::scale(model, glm::vec3(scale));
  return model;
}

inline glm::mat4 coralPlacement(std::size_t index, std::size_t total) {
  const float goldenAngle = 2.39996323f;
  const float baseRadius = 0.55f;
  const float radiusStep = 0.75f;
  const float centerLift = -0.75f;

  float angle = static_cast<float>(index) * goldenAngle;
  float radius =
      baseRadius + radiusStep * std::sqrt(static_cast<float>(index + 1));

  float x = std::cos(angle) * radius;
  float z = -0.95f - std::sin(angle) * radius;
  float y = centerLift + 0.08f * static_cast<float>(index % 4) -
            0.04f * static_cast<float>(total);

  float rotation = angle * 0.22f;
  float scale = 1.10f + 0.06f * static_cast<float>(index % 4);

  glm::mat4 model = glm::mat4(1.0f);
  model = glm::translate(model, glm::vec3(x, y, z));
  model = glm::rotate(model, rotation, glm::vec3(0.0f, 1.0f, 0.0f));
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

inline std::vector<Renderable> loadSceneModels() {
  namespace fs = std::filesystem;

  std::vector<fs::path> modelFiles;
  for (const auto &entry : fs::directory_iterator("models")) {
    if (!entry.is_regular_file()) {
      continue;
    }

    const std::string extension = entry.path().extension().string();
    if (extension == ".obj" || extension == ".OBJ" || extension == ".fbx" ||
        extension == ".FBX" || extension == ".gltf" || extension == ".GLTF" ||
        extension == ".glb" || extension == ".GLB" || extension == ".dae" ||
        extension == ".DAE") {
      modelFiles.push_back(entry.path());
    }
  }

  std::sort(modelFiles.begin(), modelFiles.end());

  std::vector<Renderable> renderables;
  std::size_t coralIndex = 0;
  std::size_t backgroundIndex = 0;
  std::size_t backgroundTotal = 0;
  for (const auto &file : modelFiles) {
    if (!isCoralModel(file)) {
      ++backgroundTotal;
    }
  }

  for (const auto &file : modelFiles) {
    glm::mat4 transform;
    if (isCoralModel(file)) {
      transform = coralPlacement(coralIndex++, modelFiles.size());
    } else if (isFishModel(file)) {
      transform = fishPlacement(coralIndex, backgroundTotal + 1);
      ++backgroundIndex;
    } else {
      transform = backgroundPlacement(backgroundIndex++, backgroundTotal);
    }

    std::string stem = file.stem().string();
    std::transform(stem.begin(), stem.end(), stem.begin(), [](unsigned char c) {
      return static_cast<char>(std::tolower(c));
    });
    if (stem.find("fishes") != std::string::npos) {
      transform = glm::scale(transform, glm::vec3(0.25f));
    }

    std::vector<Renderable> loaded = loadModelFile(file, transform);
    renderables.insert(renderables.end(), loaded.begin(), loaded.end());
  }

  if (renderables.empty()) {
    std::cerr << "No renderable models were loaded from models/" << std::endl;
  }

  return renderables;
}

inline void destroyRenderables(std::vector<Renderable> &renderables) {
  for (Renderable &renderable : renderables) {
    glDeleteBuffers(1, &renderable.EBO);
    glDeleteBuffers(1, &renderable.VBO);
    glDeleteVertexArrays(1, &renderable.VAO);
  }
  renderables.clear();
}
