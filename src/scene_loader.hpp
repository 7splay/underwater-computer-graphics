#pragma once

#include <filesystem>
#include <functional>
#include <iostream>
#include <string>
#include <unordered_map>
#include <vector>

#include "coral_placement.hpp"
#include "instancing.hpp"
#include "models.hpp"

inline std::vector<Renderable>
loadSceneModels(const std::function<void()> &onProgress = {}) {
  namespace fs = std::filesystem;

  auto tick = [&]() {
    if (onProgress) {
      onProgress();
    }
  };

  constexpr std::size_t kFishInstancesPerModel = 14;
  constexpr std::size_t kFishesInstancesPerModel = 10;

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

  std::unordered_map<std::string, std::vector<Renderable>> meshCache;
  auto loadBaseMeshes = [&](const fs::path &file) -> std::vector<Renderable> & {
    const std::string key = file.string();
    auto found = meshCache.find(key);
    if (found != meshCache.end()) {
      return found->second;
    }
    std::cout << "Loading model: " << file.filename().string() << "..."
              << std::endl;
    tick();
    std::vector<Renderable> meshes = loadModelFile(file, glm::mat4(1.0f));
    meshCache[key] = std::move(meshes);
    tick();
    return meshCache[key];
  };

  std::vector<Renderable> renderables;
  std::size_t coralIndex = 0;
  std::size_t fishInstanceIndex = 0;
  std::size_t backgroundIndex = 0;
  std::size_t backgroundTotal = 0;
  for (const auto &file : modelFiles) {
    if (!isCoralModel(file)) {
      ++backgroundTotal;
    }
  }

  for (const auto &file : modelFiles) {
    std::string stem = file.stem().string();
    std::transform(stem.begin(), stem.end(), stem.begin(), [](unsigned char c) {
      return static_cast<char>(std::tolower(c));
    });

    if (isCoralModel(file)) {
      const glm::mat4 transform =
          coralPlacement(coralIndex++, modelFiles.size());
      const std::vector<Renderable> &baseMeshes = loadBaseMeshes(file);
      for (const Renderable &mesh : baseMeshes) {
        renderables.push_back(cloneRenderable(mesh, transform));
      }
      continue;
    }

    if (isFishModel(file)) {
      const std::size_t instanceCount = stem.find("fishes") != std::string::npos
                                            ? kFishesInstancesPerModel
                                            : kFishInstancesPerModel;
      const std::vector<Renderable> &baseMeshes = loadBaseMeshes(file);
      for (std::size_t instance = 0; instance < instanceCount; ++instance) {
        glm::mat4 transform = fishPlacement(fishInstanceIndex++, backgroundTotal);
        if (stem.find("fishes") != std::string::npos) {
          transform = glm::scale(transform, glm::vec3(0.25f));
        }
        for (const Renderable &mesh : baseMeshes) {
          renderables.push_back(cloneRenderable(mesh, transform));
        }
      }
      ++backgroundIndex;
      continue;
    }

    if (isShipModel(file)) {
      const glm::mat4 transform = sunkenShipPlacement();
      std::vector<Renderable> loaded = loadModelFile(file, transform);
      renderables.insert(renderables.end(), loaded.begin(), loaded.end());
      ++backgroundIndex;
      continue;
    }

    const glm::mat4 transform =
        backgroundPlacement(backgroundIndex++, backgroundTotal);
    std::vector<Renderable> loaded = loadModelFile(file, transform);
    renderables.insert(renderables.end(), loaded.begin(), loaded.end());
  }

  if (renderables.empty()) {
    std::cerr << "No renderable models were loaded from models/" << std::endl;
  }

  return renderables;
}
