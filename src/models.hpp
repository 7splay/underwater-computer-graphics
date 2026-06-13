#pragma once

#include <GL/glew.h>

#include <assimp/Importer.hpp>
#include <assimp/postprocess.h>
#include <assimp/scene.h>

#include <algorithm>
#include <cstddef>
#ifdef __APPLE__
#include <CoreFoundation/CoreFoundation.h>
#include <CoreGraphics/CoreGraphics.h>
#include <ImageIO/ImageIO.h>
#endif
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
  GLuint texture = 0;
  GLuint normalTexture = 0;
  bool useNormalMap = false;
  GLsizei indexCount = 0;
  glm::mat4 model = glm::mat4(1.0f);
};

struct ModelVertex {
  float position[3];
  float normal[3];
  float texCoord[2];
  float tangent[4];
};

inline std::string toLowerCopy(std::string value) {
  std::transform(value.begin(), value.end(), value.begin(), [](unsigned char c) {
    return static_cast<char>(std::tolower(c));
  });
  return value;
}

inline std::filesystem::path resolveCoralTexturePath(const std::filesystem::path &modelPath) {
  const std::string stem = toLowerCopy(modelPath.stem().string());
  const std::filesystem::path imgRoot = modelPath.parent_path().parent_path() / "img";

  auto tryPaths = [&](std::initializer_list<std::filesystem::path> candidates) {
    for (const auto &candidate : candidates) {
      if (std::filesystem::exists(candidate)) {
        return candidate;
      }
    }
    return std::filesystem::path{};
  };

  if (stem == "coral1") {
    return tryPaths({imgRoot / "coral_1" / "coral1Basecolor.jpg",
                     imgRoot / "coral_1" / "coral1Diffuse.jpg"});
  }
  if (stem == "coral2") {
    return tryPaths({imgRoot / "coral_2" / "Coral2Base.jpg",
                     imgRoot / "coral_2" / "Coral2Diffuse.jpg"});
  }
  if (stem == "coral3") {
    return tryPaths({imgRoot / "coral_3" / "CoralBase.jpg",
                     imgRoot / "coral_3" / "Coral3Diffuse.jpg"});
  }
  if (stem == "coral4") {
    return tryPaths({imgRoot / "coral_4" / "Coral4Base.jpg",
                     imgRoot / "coral_4" / "Coral4Diffuse.jpg"});
  }
  if (stem == "coral5") {
    return tryPaths({imgRoot / "coral_5" / "Coral5Base.jpg",
                     imgRoot / "coral_5" / "Coral5Diffuse.jpg"});
  }
  if (stem == "coral6") {
    return tryPaths({imgRoot / "coral_6" / "Coral6Base.jpg",
                     imgRoot / "coral_6" / "Coral6Diffuse.jpg"});
  }
  if (stem == "coral7") {
    return tryPaths({imgRoot / "coral_7" / "Coral7Base.jpg",
                     imgRoot / "coral_7" / "Coral7Diffuse.jpg"});
  }
  if (stem == "coral8") {
    return tryPaths({imgRoot / "coral_8" / "Coral8Base.jpg",
                     imgRoot / "coral_8" / "Coral8Diffuse.jpg"});
  }
  if (stem == "coral9") {
    return tryPaths({imgRoot / "coral_9" / "Coral9Base.jpg",
                     imgRoot / "coral_9" / "Coral9Diffuse.jpg"});
  }
  if (stem == "coral10") {
    return tryPaths({imgRoot / "coral_10" / "Coral10Base.jpg",
                     imgRoot / "coral_10" / "Coral10Diffuse.jpg"});
  }
  if (stem == "coral11") {
    return tryPaths({imgRoot / "coral_11" / "Coral11Base.jpg",
                     imgRoot / "coral_11" / "Coral11Diffuse.jpg"});
  }

  if (stem == "ship") {
    return tryPaths({imgRoot / "ship" / "ship.png",
                     imgRoot / "ship" / "texture.png"});
  }

  if (stem == "shark") {
    return tryPaths({imgRoot / "shark" / "Nurse_Shark_Quad_Diffuse.png",
                     imgRoot / "shark" / "Nurse_Shark_Tris_Diffuse.png"});
  }

  if (stem == "fish") {
    return tryPaths({imgRoot / "fish" / "Tailor_low_DefaultMaterial_BaseColor.png",
                     imgRoot / "fish" / "Tailor_low_DefaultMaterial_BaseColor.png",
                     imgRoot / "fish_DefaultMaterial_BaseColor.png",
                     imgRoot / "fish" / "texture.png"});
  }

  if (stem == "fishes") {
    return tryPaths({imgRoot / "fishes" / "Striped-Dottyback-Pseudochromis-sankeyi.jpg",
                     imgRoot / "fishes" / "fishes.jpg"});
  }

  return {};
}

inline std::filesystem::path resolveModelNormalTexturePath(
    const std::filesystem::path &modelPath) {
  const std::string stem = toLowerCopy(modelPath.stem().string());
  const std::filesystem::path imgRoot = modelPath.parent_path().parent_path() / "img";

  auto tryPaths = [&](std::initializer_list<std::filesystem::path> candidates) {
    for (const auto &candidate : candidates) {
      if (std::filesystem::exists(candidate)) {
        return candidate;
      }
    }
    return std::filesystem::path{};
  };

  if (stem == "coral1") {
    return tryPaths({imgRoot / "coral_1" / "coral1Normals.jpg"});
  }
  if (stem == "coral2") {
    return tryPaths({imgRoot / "coral_2" / "Coral2Normals.jpg"});
  }
  if (stem == "coral3") {
    return tryPaths({imgRoot / "coral_3" / "Coral3Normals.jpg"});
  }
  if (stem == "coral4") {
    return tryPaths({imgRoot / "coral_4" / "Coral4Normals.jpg"});
  }
  if (stem == "coral5") {
    return tryPaths({imgRoot / "coral_5" / "Coral5Normals.jpg"});
  }
  if (stem == "coral6") {
    return tryPaths({imgRoot / "coral_6" / "Coral6Normals.jpg"});
  }
  if (stem == "coral7") {
    return tryPaths({imgRoot / "coral_7" / "Coral7Normals.jpg"});
  }
  if (stem == "coral8") {
    return tryPaths({imgRoot / "coral_8" / "coral8Normals.jpg"});
  }
  if (stem == "coral9") {
    return tryPaths({imgRoot / "coral_9" / "coral9Normals.jpg"});
  }
  if (stem == "coral10") {
    return tryPaths({imgRoot / "coral_10" / "coral10Normals.jpg"});
  }
  if (stem == "coral11") {
    return tryPaths({imgRoot / "coral_11" / "coral12Normals.jpg",
                     imgRoot / "coral_11" / "coral11Normals.jpg"});
  }

  if (stem == "fish") {
    return tryPaths({imgRoot / "fish" / "Tailor_low_DefaultMaterial_Normal.png"});
  }

  if (stem == "shark") {
    return tryPaths({imgRoot / "shark" / "Nurse_Shark_Quad_Normal.png",
                     imgRoot / "shark" / "Nurse_Shark_Tris_Normal.png"});
  }

  return {};
}

#ifdef __APPLE__
inline GLuint loadTextureFromFile(const std::filesystem::path &texturePath) {
  if (!std::filesystem::exists(texturePath)) {
    return 0;
  }

  CFStringRef pathString = CFStringCreateWithCString(
      kCFAllocatorDefault, texturePath.string().c_str(), kCFStringEncodingUTF8);
  if (pathString == nullptr) {
    return 0;
  }

  CFURLRef url = CFURLCreateWithFileSystemPath(kCFAllocatorDefault, pathString,
                                              kCFURLPOSIXPathStyle, false);
  CFRelease(pathString);
  if (url == nullptr) {
    return 0;
  }

  CGImageSourceRef source = CGImageSourceCreateWithURL(url, nullptr);
  CFRelease(url);
  if (source == nullptr) {
    return 0;
  }

  CGImageRef image = CGImageSourceCreateImageAtIndex(source, 0, nullptr);
  CFRelease(source);
  if (image == nullptr) {
    return 0;
  }

  const size_t width = CGImageGetWidth(image);
  const size_t height = CGImageGetHeight(image);
  if (width == 0 || height == 0) {
    CGImageRelease(image);
    return 0;
  }

  std::vector<unsigned char> pixels(width * height * 4u, 0u);
  CGColorSpaceRef colorSpace = CGColorSpaceCreateDeviceRGB();
  CGContextRef context = CGBitmapContextCreate(
      pixels.data(), width, height, 8, width * 4u, colorSpace,
      kCGImageAlphaPremultipliedLast | kCGBitmapByteOrder32Big);
  CGColorSpaceRelease(colorSpace);
  if (context == nullptr) {
    CGImageRelease(image);
    return 0;
  }

  CGContextTranslateCTM(context, 0.0f, static_cast<CGFloat>(height));
  CGContextScaleCTM(context, 1.0f, -1.0f);
  CGContextDrawImage(context, CGRectMake(0, 0, width, height), image);
  CGContextRelease(context);
  CGImageRelease(image);

  GLuint texture = 0;
  glGenTextures(1, &texture);
  glBindTexture(GL_TEXTURE_2D, texture);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, static_cast<GLsizei>(width),
               static_cast<GLsizei>(height), 0, GL_RGBA, GL_UNSIGNED_BYTE,
               pixels.data());
  glGenerateMipmap(GL_TEXTURE_2D);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
  glBindTexture(GL_TEXTURE_2D, 0);
  return texture;
}
#endif

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
      const glm::vec3 tangent(vertex.tangent[0], vertex.tangent[1], vertex.tangent[2]);
      const glm::vec3 bitangent(mesh->mBitangents[i].x, mesh->mBitangents[i].y,
                                mesh->mBitangents[i].z);
      const glm::vec3 normal(vertex.normal[0], vertex.normal[1], vertex.normal[2]);
      const float handedness =
          (glm::dot(glm::cross(normal, tangent), bitangent) < 0.0f) ? -1.0f : 1.0f;
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

  glBindVertexArray(0);
  return true;
}

inline bool isCoralModel(const std::filesystem::path &path) {
  std::string name = path.stem().string();
  std::transform(name.begin(), name.end(), name.begin(), [](unsigned char c) {
    return static_cast<char>(std::tolower(c));
  });
  return name.find("coral") != std::string::npos;
}

inline GLuint createFallbackTexture() {
  unsigned char pixels[4] = {255u, 255u, 255u, 255u};
  GLuint texture = 0;
  glGenTextures(1, &texture);
  glBindTexture(GL_TEXTURE_2D, texture);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 1, 1, 0, GL_RGBA, GL_UNSIGNED_BYTE,
               pixels);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
  glBindTexture(GL_TEXTURE_2D, 0);
  return texture;
}

inline void loadTexturesForModel(const std::filesystem::path &modelPath,
                                 Renderable &renderable) {
  renderable.texture = 0;
  renderable.normalTexture = 0;
  renderable.useNormalMap = false;

#ifdef __APPLE__
  std::filesystem::path texturePath = resolveCoralTexturePath(modelPath);
  if (!texturePath.empty()) {
    renderable.texture = loadTextureFromFile(texturePath);
  }

  std::filesystem::path normalPath = resolveModelNormalTexturePath(modelPath);
  if (!normalPath.empty()) {
    renderable.normalTexture = loadTextureFromFile(normalPath);
    renderable.useNormalMap = renderable.normalTexture != 0;
  }
#endif

  if (renderable.texture == 0) {
    renderable.texture = createFallbackTexture();
  }
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
      loadTexturesForModel(path, renderable);
      loadedRenderables.push_back(renderable);
    }
  }

  return loadedRenderables;
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
    if (renderable.texture != 0) {
      glDeleteTextures(1, &renderable.texture);
    }
    if (renderable.normalTexture != 0) {
      glDeleteTextures(1, &renderable.normalTexture);
    }
    glDeleteBuffers(1, &renderable.EBO);
    glDeleteBuffers(1, &renderable.VBO);
    glDeleteVertexArrays(1, &renderable.VAO);
  }
  renderables.clear();
}
