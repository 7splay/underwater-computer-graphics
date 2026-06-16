#pragma once

#include "materials.hpp"
#include "vertex_layout.hpp"

#include <assimp/Importer.hpp>
#include <assimp/postprocess.h>
#include <assimp/scene.h>

#include <glm/gtc/matrix_transform.hpp>
#include <iostream>
#include <unordered_set>
#include <vector>

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

    if (mesh->HasTextureCoords(0) && mesh->mTextureCoords[0] != nullptr) {
      vertex.texCoord[0] = mesh->mTextureCoords[0][i].x;
      vertex.texCoord[1] = mesh->mTextureCoords[0][i].y;
    } else {
      vertex.texCoord[0] = vertex.position[0] * 0.5f + 0.5f;
      vertex.texCoord[1] = vertex.position[2] * 0.5f + 0.5f;
    }

    if (mesh->HasTangentsAndBitangents()) {
      vertex.tangent[0] = mesh->mTangents[i].x;
      vertex.tangent[1] = mesh->mTangents[i].y;
      vertex.tangent[2] = mesh->mTangents[i].z;
      const glm::vec3 tangent(vertex.tangent[0], vertex.tangent[1],
                              vertex.tangent[2]);
      const glm::vec3 bitangent(mesh->mBitangents[i].x, mesh->mBitangents[i].y,
                                mesh->mBitangents[i].z);
      const glm::vec3 normal(vertex.normal[0], vertex.normal[1],
                             vertex.normal[2]);
      const float handedness =
          (glm::dot(glm::cross(normal, tangent), bitangent) < 0.0f) ? -1.0f
                                                                    : 1.0f;
      vertex.tangent[3] = handedness;
    } else {
      vertex.tangent[0] = 1.0f;
      vertex.tangent[1] = 0.0f;
      vertex.tangent[2] = 0.0f;
      vertex.tangent[3] = 1.0f;
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

  bindModelVertexLayout();
  glBindVertexArray(0);
  return true;
}

inline std::vector<Renderable> loadModelFile(const std::filesystem::path &path,
                                             const glm::mat4 &modelMatrix) {
  Assimp::Importer importer;
  const aiScene *scene = importer.ReadFile(
      path.string().c_str(),
      aiProcess_Triangulate | aiProcess_GenSmoothNormals |
          aiProcess_CalcTangentSpace | aiProcess_JoinIdenticalVertices |
          aiProcess_ImproveCacheLocality);

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
      loadModelTextures(path, renderable);
      loadedRenderables.push_back(renderable);
    }
  }

  return loadedRenderables;
}

inline void destroyRenderables(std::vector<Renderable> &renderables) {
  std::unordered_set<GLuint> deletedTextures;
  std::unordered_set<GLuint> deletedBuffers;
  std::unordered_set<GLuint> deletedVertexArrays;

  for (Renderable &renderable : renderables) {
    const GLuint textures[] = {renderable.texture, renderable.normalTexture,
                               renderable.roughnessTexture,
                               renderable.metallicTexture};
    for (GLuint texture : textures) {
      if (texture != 0 && deletedTextures.insert(texture).second) {
        glDeleteTextures(1, &texture);
      }
    }
    if (renderable.EBO != 0 &&
        deletedBuffers.insert(renderable.EBO).second) {
      glDeleteBuffers(1, &renderable.EBO);
    }
    if (renderable.instVBO != 0 &&
        deletedBuffers.insert(renderable.instVBO).second) {
      glDeleteBuffers(1, &renderable.instVBO);
    }
    if (renderable.VBO != 0 &&
        deletedBuffers.insert(renderable.VBO).second) {
      glDeleteBuffers(1, &renderable.VBO);
    }
    if (renderable.VAO != 0 &&
        deletedVertexArrays.insert(renderable.VAO).second) {
      glDeleteVertexArrays(1, &renderable.VAO);
    }
  }
  renderables.clear();
}
