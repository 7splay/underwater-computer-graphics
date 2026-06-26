#pragma once

#include <algorithm>
#include <filesystem>
#include <functional>
#include <iostream>
#include <string>
#include <unordered_map>
#include <vector>

#include <glm/glm.hpp>

#include "coral_placement.hpp"
#include "lod_system.hpp"
#include "models.hpp"
#include "seaweed_cluster.hpp"
#include "seaweed_placement.hpp"

// spawn info for one fish instance, collected while the scene is loaded
// so scene.cpp can build a Fish without re-deriving the placement math
struct FishSpawn {
  std::size_t renderableIndex = 0;
  glm::vec3 position = glm::vec3(0.0f);
  float scale = 1.0f;
  std::size_t patrolIndex = 0;
};

// wrapper around LodScene that also carries fish/interactivity data the loader
// collects (coral anchors for patrol-loop generation, fish spawn info).
// fish are kept as separate per-instance renderables so the B14 AI can update
// each fish's model matrix individually; corals/seaweed/rocks use LOD groups
struct SceneLoadResult {
  LodScene lod;
  std::vector<Renderable> fishRenderables;
  std::vector<FishSpawn> fishSpawns;
  std::vector<glm::vec3> coralAnchors;  // XZ centers, used to build patrol loops
};

inline SceneLoadResult
loadSceneModels(const std::function<void()> &onProgress = {},
                float density = 1.0f) {
  namespace fs = std::filesystem;

  auto tick = [&]() {
    if (onProgress) {
      onProgress();
    }
  };

  // base per-model fish budgets, scaled by the object-density entry parameter
  const auto scaled = [density](std::size_t base) {
    return std::max<std::size_t>(1, static_cast<std::size_t>(base * density));
  };
  const std::size_t kFishInstancesPerModel = scaled(40);
  const std::size_t kFishesInstancesPerModel = scaled(24);
  const SeabedParams &seabed = kSceneSeabed;

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

  std::vector<fs::path> coralFiles;
  for (const auto &file : modelFiles) {
    if (isCoralModel(file)) {
      coralFiles.push_back(file);
    }
  }

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
  LodScene &scene = result.lod;
  auto &fishRenderables = result.fishRenderables;
  auto &fishSpawns = result.fishSpawns;
  auto &coralAnchors = result.coralAnchors;

  std::size_t coralIndex = 0;
  std::size_t backgroundIndex = 0;
  std::size_t backgroundTotal = 0;
  for (const auto &file : modelFiles) {
    if (!isCoralModel(file)) {
      ++backgroundTotal;
    }
  }

  CoralPlacementResult coralPlacements{};
  if (!coralFiles.empty()) {
    coralPlacements =
        generateNoiseCoralPlacements(coralFiles.size(), seabed, density);
  }

  const Renderable clusterSeaweed = loadClusterSeaweed();
  if (clusterSeaweed.VAO != 0) {
    const std::vector<glm::mat4> clusterTransforms =
        generateClusterSeaweedPlacements(seabed, density);
    std::cout << "Placed " << clusterTransforms.size()
              << " cluster seaweed using noise scattering." << std::endl;
    if (!clusterTransforms.empty()) {
      // seaweed skips the shadow pass: alpha-cutout depth writes are the most
      // expensive per-fragment work in the shadow map, and the thin blades
      // cast no visible shadow on sand anyway
      LodInstancedGroup group = createLodInstancedGroup(
          clusterSeaweed, clusterTransforms, 16.0f, 50.0f, 1.2f, 1.6f, false);
      group.low.useOpacityCutout = false;
      group.low.useAlphaCutout  = false;
      scene.groups.push_back(std::move(group));
    }
  }

  if (!coralFiles.empty()) {
    for (std::size_t coralIndex = 0; coralIndex < coralFiles.size();
         ++coralIndex) {
      const fs::path &file = coralFiles[coralIndex];
      const std::vector<glm::mat4> &transforms =
          coralPlacements.perVariantTransforms[coralIndex];
      if (transforms.empty()) {
        continue;
      }
      // collect one anchor per coral variant (colony center) so scene.cpp can
      // build a Catmull-Rom patrol loop around each colony. every-coral anchors
      // would create ~110 patrol loops and overload the fish AI grid
      if (!transforms.empty()) {
        coralAnchors.push_back(glm::vec3(transforms.front()[3]));
      }

      const std::vector<Renderable> &baseMeshes = loadBaseMeshes(file);
      for (const Renderable &mesh : baseMeshes) {
        LodInstancedGroup group = createLodInstancedGroup(
            mesh, transforms, 18.0f, 40.0f, 0.75f, 0.9f, true);
        group.low.useOpacityCutout = false;
        group.low.useAlphaCutout   = false;
        scene.groups.push_back(std::move(group));
      }
    }
  }

  std::size_t fishInstanceIndex = 0;
  for (const auto &file : modelFiles) {
    std::string stem = file.stem().string();
    std::transform(stem.begin(), stem.end(), stem.begin(), [](unsigned char c) {
      return static_cast<char>(std::tolower(c));
    });

    if (isCoralModel(file)) {
      // corals are processed in a dedicated LOD pass above; skip in-loop
      continue;
    }

    if (isSkippedModel(file)) {
      continue;
    }

    if (isFishModel(file)) {
      // fish are kept as separate per-instance renderables (not LOD groups) so
      // the B14 AI can update each fish's model matrix every frame. corals and
      // seaweed use LOD; fish do not. placement uses colleague's noise-driven
      // fishPlacementNoise for consistency with the rest of the scene
      const bool isSchool = stem.find("fishes") != std::string::npos;
      const std::size_t instanceCount =
          isSchool ? kFishesInstancesPerModel : kFishInstancesPerModel;
      const float meshScale = isSchool ? 0.25f : 1.0f;
      const std::vector<Renderable> &baseMeshes = loadBaseMeshes(file);
      // fishes.obj bundles 5 sub-meshes (a pre-baked school). clone them all
      // per instance would scatter ghost fish; keep only the largest sub-mesh
      // as the canonical single-fish mesh
      const Renderable *primaryMesh = nullptr;
      for (const Renderable &mesh : baseMeshes) {
        if (primaryMesh == nullptr || mesh.indexCount > primaryMesh->indexCount) {
          primaryMesh = &mesh;
        }
      }
      if (primaryMesh == nullptr) {
        ++backgroundIndex;
        continue;
      }
      for (std::size_t instance = 0; instance < instanceCount; ++instance) {
        glm::mat4 transform =
            fishPlacementNoise(fishInstanceIndex++, seabed, meshScale);
        const float placementScale = glm::length(transform[0]);
        const glm::vec3 pos(transform[3]);
        // patrol assignment is deferred to scene.cpp where the actual patrol
        // loops are built (decoupled from coral placement since corals moved
        // to one corner of the playable area)
        const std::size_t idx = fishRenderables.size();
        fishRenderables.push_back(cloneRenderable(*primaryMesh, transform));
        FishSpawn spawn;
        spawn.renderableIndex = idx;
        spawn.position = pos;
        spawn.scale = placementScale;
        spawn.patrolIndex = 0;
        fishSpawns.push_back(spawn);
      }
      ++backgroundIndex;
      continue;
    }

    if (isSharkModel(file)) {
      const std::vector<Renderable> &baseMeshes = loadBaseMeshes(file);
      if (!baseMeshes.empty()) {
        const std::vector<glm::mat4> transforms = {sharkPlacement(seabed)};
        LodInstancedGroup group = createLodInstancedGroup(
            baseMeshes.front(), transforms, 22.0f, 40.0f, 2.4f, 1.2f, false,
            false, 3, 8);
        scene.groups.push_back(std::move(group));
      }
      ++backgroundIndex;
      continue;
    }

    if (isShipModel(file)) {
      const glm::mat4 transform = sunkenShipPlacement(seabed);
      std::vector<Renderable> loaded = loadModelFile(file, transform);
      scene.staticRenderables.insert(scene.staticRenderables.end(),
                                     loaded.begin(), loaded.end());
      ++backgroundIndex;
      continue;
    }

    const glm::mat4 transform =
        backgroundPlacement(backgroundIndex++, backgroundTotal);
    std::vector<Renderable> loaded = loadModelFile(file, transform);
    scene.staticRenderables.insert(scene.staticRenderables.end(), loaded.begin(),
                                   loaded.end());
  }

  if (scene.groups.empty() && scene.staticRenderables.empty()) {
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
