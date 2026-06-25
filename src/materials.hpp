#pragma once

#include "renderable.hpp"
#include "texture_paths.hpp"
#include "textures.hpp"

#include <glm/glm.hpp>

inline bool isCoralModel(const std::filesystem::path &path) {
  return toLowerCopy(path.stem().string()).find("coral") != std::string::npos;
}

inline bool isFishModel(const std::filesystem::path &path) {
  const std::string name = toLowerCopy(path.stem().string());
  if (name.find("shark") != std::string::npos) {
    return false;
  }
  return name.find("fish") != std::string::npos ||
         name.find("fishes") != std::string::npos;
}

// returns true for any model that should be skipped entirely (not rendered)
inline bool isSkippedModel(const std::filesystem::path &path) {
  const std::string name = toLowerCopy(path.stem().string());
  // worm is loaded manually as the bait mesh, never auto-placed in the scene
  // shark is rendered via a dedicated LOD group in scene_loader, not skipped
  return name.find("worm") != std::string::npos;
}

inline bool isSharkModel(const std::filesystem::path &path) {
  return toLowerCopy(path.stem().string()).find("shark") != std::string::npos;
}

inline bool isShipModel(const std::filesystem::path &path) {
  return toLowerCopy(path.stem().string()).find("ship") != std::string::npos;
}

// Default metallic/roughness when a map is missing.
inline void applyMaterialPreset(const std::filesystem::path &modelPath,
                                Renderable &renderable) {
  if (isCoralModel(modelPath)) {
    renderable.metallic = 0.0f;
    renderable.roughness = 0.72f;
    renderable.normalStrength = 1.0f;
    return;
  }
  if (isFishModel(modelPath)) {
    renderable.metallic = 0.04f;
    renderable.roughness = 0.42f;
    renderable.normalStrength = 1.0f;
    // fish meshes are flat planes with alpha-cutout fins, discard transparent
    // pixels so the silhouette reads cleanly without blobby transparent edges
    renderable.useAlphaTest = true;
    renderable.alphaCutoff = 0.5f;
    return;
  }
  if (isShipModel(modelPath)) {
    renderable.metallic = 0.82f;
    renderable.roughness = 0.58f;
    renderable.albedoTint = glm::vec3(0.58f, 0.64f, 0.56f);
    return;
  }
}

inline void loadModelTextures(const std::filesystem::path &modelPath,
                              Renderable &renderable) {
  renderable.texture = 0;
  renderable.normalTexture = 0;
  renderable.roughnessTexture = 0;
  renderable.metallicTexture = 0;
  renderable.useNormalMap = false;
  renderable.useRoughnessMap = false;
  renderable.useMetallicMap = false;
  renderable.useArmMap = false;
  applyMaterialPreset(modelPath, renderable);

  const std::filesystem::path diffusePath = resolveDiffuseTexturePath(modelPath);
  if (!diffusePath.empty()) {
    renderable.texture = loadTextureFromFile(diffusePath);
  }

  const std::filesystem::path normalPath = resolveNormalTexturePath(modelPath);
  if (!normalPath.empty()) {
    renderable.normalTexture = loadTextureFromFile(normalPath);
    renderable.useNormalMap = renderable.normalTexture != 0;
  }

  const std::filesystem::path roughnessPath =
      resolveRoughnessTexturePath(modelPath);
  if (!roughnessPath.empty()) {
    renderable.roughnessTexture = loadTextureFromFile(roughnessPath);
    renderable.useRoughnessMap = renderable.roughnessTexture != 0;
  }

  const std::filesystem::path metallicPath =
      resolveMetallicTexturePath(modelPath);
  if (!metallicPath.empty()) {
    renderable.metallicTexture = loadTextureFromFile(metallicPath);
    renderable.useMetallicMap = renderable.metallicTexture != 0;
  }

  if (renderable.texture == 0) {
    renderable.texture = createFallbackTexture();
  }
}

// Sand: normal map + optional ARM (G=roughness, B=metallic).
inline void loadSandTextures(Renderable &sand) {
  const std::filesystem::path sandDir = "img/sand";

  sand.texture = 0;
  sand.normalTexture = 0;
  sand.roughnessTexture = 0;
  sand.metallicTexture = 0;
  sand.useNormalMap = false;
  sand.useRoughnessMap = false;
  sand.useMetallicMap = false;
  sand.useArmMap = false;
  sand.metallic = 0.0f;
  sand.roughness = 0.92f;
  sand.normalStrength = 1.8f;
  sand.albedoTint = glm::vec3(0.68f, 0.82f, 0.90f);

  const std::filesystem::path diffusePath =
      sandDir / "coral_ground_02_diff_4k.jpg";
  const std::filesystem::path normalPath =
      sandDir / "coral_ground_02_nor_gl_4k.jpg";
  const std::filesystem::path armPath = sandDir / "damp_sand_arm_4k.jpg";

  if (std::filesystem::exists(diffusePath)) {
    sand.texture = loadTextureFromFile(diffusePath);
  }
  if (std::filesystem::exists(normalPath)) {
    sand.normalTexture = loadTextureFromFile(normalPath);
    sand.useNormalMap = sand.normalTexture != 0;
  }
  if (std::filesystem::exists(armPath)) {
    sand.roughnessTexture = loadTextureFromFile(armPath);
    sand.metallicTexture = sand.roughnessTexture;
    sand.useArmMap = sand.roughnessTexture != 0;
    sand.useRoughnessMap = sand.useArmMap;
    sand.useMetallicMap = sand.useArmMap;
  }

  if (sand.texture == 0) {
    sand.texture = createFallbackTexture();
  }
}
