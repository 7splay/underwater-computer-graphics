#pragma once

#include <filesystem>
#include <functional>
#include <iostream>
#include <limits>
#include <string>
#include <unordered_map>
#include <vector>

#include <glm/glm.hpp>

#include "coral_placement.hpp"
#include "instancing.hpp"
#include "models.hpp"

// spawn info for one fish instance, collected while the scene is loaded
// so scene.cpp can build a Fish without re-deriving the placement math
struct FishSpawn {
  std::size_t renderableIndex = 0;
  glm::vec3 position = glm::vec3(0.0f);
  float scale = 1.0f;
  std::size_t patrolIndex = 0;
};

struct SceneLoadResult {
  std::vector<Renderable> renderables;
  std::vector<FishSpawn> fishSpawns;
  std::vector<glm::vec3> coralAnchors;  // XZ centers, used to build patrol loops
};

inline SceneLoadResult
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

  SceneLoadResult result;
  auto &renderables = result.renderables;
  auto &fishSpawns = result.fishSpawns;
  auto &coralAnchors = result.coralAnchors;

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
      coralAnchors.push_back(glm::vec3(transform[3]));
      continue;
    }

    if (isSkippedModel(file)) {
      continue;
    }

    if (isFishModel(file)) {
      const std::size_t instanceCount =
          stem.find("fishes") != std::string::npos ? kFishesInstancesPerModel
                                                   : kFishInstancesPerModel;
      const float meshScale = stem.find("fishes") != std::string::npos ? 0.25f : 1.0f;
      const std::vector<Renderable> &baseMeshes = loadBaseMeshes(file);
      for (std::size_t instance = 0; instance < instanceCount; ++instance) {
        glm::mat4 transform = fishPlacement(fishInstanceIndex++, backgroundTotal);
        transform = glm::scale(transform, glm::vec3(meshScale));
        const float placementScale = glm::length(transform[0]);
        const glm::vec3 pos(transform[3]);
        // assign each fish to its nearest coral anchor's patrol loop, so the
        // school breaks into smaller groups orbiting different coral clusters
        // instead of clustering on a single loop
        std::size_t patrol = 0;
        if (!coralAnchors.empty()) {
          float bestDist = std::numeric_limits<float>::max();
          for (std::size_t a = 0; a < coralAnchors.size(); ++a) {
            const glm::vec2 d2(pos.x - coralAnchors[a].x, pos.z - coralAnchors[a].z);
            const float d = glm::dot(d2, d2);
            if (d < bestDist) {
              bestDist = d;
              patrol = a;
            }
          }
        }
        // some fish assets (fishes.obj) bundle many sub-meshes (a pre-baked
        // school of 5 fish at offsets in model space). cloning every sub-mesh
        // per instance would scatter 4 ghost fish around each spawn point, so
        // we keep only the largest sub-mesh as the canonical single-fish mesh
        const Renderable *primaryMesh = nullptr;
        for (const Renderable &mesh : baseMeshes) {
          if (primaryMesh == nullptr ||
              mesh.indexCount > primaryMesh->indexCount) {
            primaryMesh = &mesh;
          }
        }
        if (primaryMesh != nullptr) {
          const std::size_t idx = renderables.size();
          renderables.push_back(cloneRenderable(*primaryMesh, transform));
          FishSpawn spawn;
          spawn.renderableIndex = idx;
          spawn.position = pos;
          spawn.scale = placementScale;
          spawn.patrolIndex = patrol;
          fishSpawns.push_back(spawn);
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

  return result;
}

// bait mesh loader: same "Loading model:" log + progress callback as
// loadBaseMeshes above, but returns the single largest sub-mesh instead of all
// sub-meshes (worm.obj has Blender debug junk we need to discard)
inline Renderable loadBaitMesh(const std::string &filename,
                               const std::function<void()> &onProgress = {}) {
  std::cout << "Loading model: " << filename << "..." << std::endl;
  if (onProgress) onProgress();
  std::vector<Renderable> meshes =
      loadModelFile(std::filesystem::path("models") / filename, glm::mat4(1.0f));
  if (onProgress) onProgress();
  const Renderable *best = nullptr;
  for (const Renderable &m : meshes) {
    if (best == nullptr || m.indexCount > best->indexCount) best = &m;
  }
  return best != nullptr ? *best : Renderable{};
}
