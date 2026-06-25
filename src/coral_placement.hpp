#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <utility>
#include <vector>

#include "seabed_height.hpp"

namespace {
constexpr float kCoralMeshBaseY = 0.476f;
constexpr float kGoldenAngle = 2.39996323f;

constexpr float kDensityOffsetX = 137.31f;
constexpr float kDensityOffsetZ = 419.73f;
constexpr float kPatchScale = 19.0f;

constexpr float kShipAnchorX = 0.0f;
constexpr float kShipAnchorZ = -22.0f;
constexpr float kSceneHalfExtent = 54.0f;
constexpr std::size_t kMaxTotalCorals = 50;
constexpr float kShipColonyMinDist = 5.5f;
constexpr float kShipColonyNearMaxDist = 18.0f;
constexpr float kMaxNearReefColonies = 6.0f;
constexpr float kMinColonySeparation = 6.0f;
constexpr float kColonyCoralsMin = 4.0f;
constexpr float kColonyCoralsMax = 6.0f;
constexpr float kColonySpread = 1.85f;
constexpr float kScatterGridStep = 7.0f;
constexpr float kScatterMinSpacing = 4.0f;
constexpr float kScatterPatchThreshold = 0.50f;
}  // namespace

inline float coralPatchNoise(float worldX, float worldZ) {
  return seabedNoise.octave2D_01(
      (worldX + kDensityOffsetX) / kPatchScale,
      (worldZ + kDensityOffsetZ) / kPatchScale, 3);
}

inline float coralDetailNoise(float worldX, float worldZ, float channel) {
  return seabedNoise.noise2D_01(worldX * 0.17f + channel * 41.0f,
                                worldZ * 0.21f + channel * 19.0f);
}

inline float coralVariantNoise(float worldX, float worldZ) {
  return seabedNoise.octave2D_01(worldX * 0.063f + 17.3f,
                                 worldZ * 0.051f + 9.7f, 2);
}

inline float sampleSeabedSlope(float worldX, float worldZ,
                               const SeabedParams &seabed) {
  const float h = sampleSeabedWorldHeight(worldX, worldZ, seabed);
  const float hx =
      sampleSeabedWorldHeight(worldX + 0.45f, worldZ, seabed) - h;
  const float hz =
      sampleSeabedWorldHeight(worldX, worldZ + 0.45f, seabed) - h;
  return std::sqrt(hx * hx + hz * hz);
}

inline bool isFarEnoughFromPoints(const glm::vec2 &point,
                                  const std::vector<glm::vec2> &others,
                                  float minDistance) {
  for (const glm::vec2 &other : others) {
    if (glm::length(point - other) < minDistance) {
      return false;
    }
  }
  return true;
}

inline glm::mat4 buildCoralTransform(float x, float z, float yRotation,
                                     float scale, float colonyStrength,
                                     const SeabedParams &seabed) {
  const float seabedY = sampleSeabedWorldHeight(x, z, seabed);
  const float hx =
      sampleSeabedWorldHeight(x + 0.42f, z, seabed) - seabedY;
  const float hz =
      sampleSeabedWorldHeight(x, z + 0.42f, seabed) - seabedY;

  const float sink =
      0.08f + coralDetailNoise(x, z, 7.0f) * 0.10f + colonyStrength * 0.04f;
  const float y =
      seabedY + kCoralMeshBaseY * scale * (1.0f - sink) + 0.015f;

  const float pitch = std::atan2(-hx, 1.55f);
  const float roll = std::atan2(hz, 1.55f);

  glm::mat4 model = glm::mat4(1.0f);
  model = glm::translate(model, glm::vec3(x, y, z));
  model = glm::rotate(model, yRotation, glm::vec3(0.0f, 1.0f, 0.0f));
  model = glm::rotate(model, pitch, glm::vec3(1.0f, 0.0f, 0.0f));
  model = glm::rotate(model, roll, glm::vec3(0.0f, 0.0f, 1.0f));
  model = glm::scale(model, glm::vec3(scale));
  return model;
}

struct CoralPlacementResult {
  std::vector<std::vector<glm::mat4>> perVariantTransforms;
  std::size_t totalCount = 0;
};

inline void addReefColony(const glm::vec2 &center, std::size_t variantCount,
                          const SeabedParams &seabed,
                          CoralPlacementResult &result) {
  if (result.totalCount >= kMaxTotalCorals) {
    return;
  }
  if (sampleSeabedSlope(center.x, center.y, seabed) > 1.45f) {
    return;
  }

  const float colonyAngle =
      coralDetailNoise(center.x, center.y, 1.0f) * glm::two_pi<float>();
  const std::size_t primaryVariant =
      static_cast<std::size_t>(coralVariantNoise(center.x, center.y) *
                               static_cast<float>(variantCount)) %
      variantCount;
  const std::size_t secondaryVariant =
      (primaryVariant + 2U +
       static_cast<std::size_t>(coralDetailNoise(center.x, center.y, 2.0f) *
                                3.0f)) %
      variantCount;
  const float colonyStrength = coralPatchNoise(center.x, center.y);

  const int coralCount =
      static_cast<int>(kColonyCoralsMin +
                       coralDetailNoise(center.x, center.y, 3.0f) *
                           (kColonyCoralsMax - kColonyCoralsMin + 0.99f));

  for (int i = 0; i < coralCount; ++i) {
    if (result.totalCount >= kMaxTotalCorals) {
      break;
    }
    const float ringAngle =
        colonyAngle + static_cast<float>(i) * kGoldenAngle +
        (coralDetailNoise(center.x + static_cast<float>(i), center.y, 4.0f) -
         0.5f) *
            0.45f;
    const float ringRadius =
        (i == 0 ? 0.0f : 0.18f) +
        std::sqrt(static_cast<float>(i)) * 0.42f +
        (coralDetailNoise(center.x, center.y + static_cast<float>(i), 5.0f) -
         0.5f) *
            0.22f;
    const float clampedRadius =
        std::min(ringRadius, kColonySpread);
    const float worldX =
        center.x + std::cos(ringAngle) * clampedRadius;
    const float worldZ =
        center.y + std::sin(ringAngle) * clampedRadius;

    if (sampleSeabedSlope(worldX, worldZ, seabed) > 1.45f) {
      continue;
    }

    const float roleBlend = static_cast<float>(i) /
                            std::max(static_cast<float>(coralCount - 1), 1.0f);
    const float scale =
        (i == 0 ? 1.12f : 0.88f - roleBlend * 0.18f) +
        (coralDetailNoise(worldX, worldZ, 6.0f) - 0.5f) * 0.10f;
    const float rotation =
        colonyAngle +
        (coralDetailNoise(worldX, worldZ, 8.0f) - 0.5f) * 0.65f;

    const std::size_t variant =
        (i % 3 == 0) ? secondaryVariant : primaryVariant;

    result.perVariantTransforms[variant].push_back(buildCoralTransform(
        worldX, worldZ, rotation, scale, colonyStrength, seabed));
    ++result.totalCount;
  }
}

inline void collectColonyCandidates(
    float minDist, float maxDist, float preferNearShipWeight,
    std::vector<std::pair<glm::vec2, float>> &out,
    const SeabedParams &seabed) {
  for (int seed = 0; seed < 64; ++seed) {
    const float seedF = static_cast<float>(seed);
    const float angle =
        coralDetailNoise(seedF * 1.91f + minDist, kShipAnchorZ + maxDist,
                         20.0f) *
        glm::two_pi<float>();
    const float dist =
        minDist + coralDetailNoise(seedF * 2.37f + maxDist, kShipAnchorX, 21.0f) *
                      (maxDist - minDist);
    const glm::vec2 center(kShipAnchorX + std::cos(angle) * dist,
                           kShipAnchorZ + std::sin(angle) * dist);

    if (coralPatchNoise(center.x, center.y) < 0.44f) {
      continue;
    }
    if (sampleSeabedSlope(center.x, center.y, seabed) > 1.45f) {
      continue;
    }

    const float distScore =
        preferNearShipWeight > 0.5f
            ? 1.0f - glm::smoothstep(minDist, maxDist, dist)
            : glm::smoothstep(minDist, maxDist, dist);
    const float score =
        coralPatchNoise(center.x, center.y) * 0.55f + distScore * 0.45f;
    out.emplace_back(center, score);
  }
}

inline void pickColonyCenters(
    std::vector<std::pair<glm::vec2, float>> candidates, float maxCount,
    std::vector<glm::vec2> &colonyCenters) {
  std::sort(candidates.begin(), candidates.end(),
            [](const std::pair<glm::vec2, float> &a,
               const std::pair<glm::vec2, float> &b) { return a.second > b.second; });

  for (const auto &candidate : candidates) {
    if (colonyCenters.size() >= static_cast<std::size_t>(maxCount)) {
      break;
    }
    if (!isFarEnoughFromPoints(candidate.first, colonyCenters,
                               kMinColonySeparation)) {
      continue;
    }
    colonyCenters.push_back(candidate.first);
  }
}

inline bool isWithinScene(float worldX, float worldZ) {
  return worldX >= -kSceneHalfExtent && worldX <= kSceneHalfExtent &&
         worldZ >= -kSceneHalfExtent && worldZ <= kSceneHalfExtent;
}

inline void addScatteredCoral(float worldX, float worldZ, std::size_t variantCount,
                              const SeabedParams &seabed, float colonyStrength,
                              CoralPlacementResult &result) {
  if (result.totalCount >= kMaxTotalCorals) {
    return;
  }
  if (!isWithinScene(worldX, worldZ)) {
    return;
  }
  if (coralPatchNoise(worldX, worldZ) < kScatterPatchThreshold) {
    return;
  }
  if (sampleSeabedSlope(worldX, worldZ, seabed) > 1.45f) {
    return;
  }

  const std::size_t variant =
      static_cast<std::size_t>(coralVariantNoise(worldX, worldZ) *
                               static_cast<float>(variantCount)) %
      variantCount;
  const float scale =
      0.78f + coralDetailNoise(worldX, worldZ, 9.0f) * 0.28f;
  const float rotation =
      coralDetailNoise(worldX, worldZ, 10.0f) * glm::two_pi<float>();

  result.perVariantTransforms[variant].push_back(buildCoralTransform(
      worldX, worldZ, rotation, scale, colonyStrength, seabed));
  ++result.totalCount;

  if (result.totalCount >= kMaxTotalCorals) {
    return;
  }
  if (coralPatchNoise(worldX + 0.3f, worldZ + 0.3f) < 0.72f) {
    return;
  }

  const float offsetAngle =
      coralDetailNoise(worldX, worldZ, 11.0f) * glm::two_pi<float>();
  const float offsetRadius =
      0.55f + coralDetailNoise(worldX, worldZ, 12.0f) * 0.75f;
  const float companionX = worldX + std::cos(offsetAngle) * offsetRadius;
  const float companionZ = worldZ + std::sin(offsetAngle) * offsetRadius;

  if (coralPatchNoise(companionX, companionZ) < kScatterPatchThreshold ||
      sampleSeabedSlope(companionX, companionZ, seabed) > 1.45f) {
    return;
  }

  const std::size_t companionVariant =
      (variant + 1U +
       static_cast<std::size_t>(coralDetailNoise(companionX, companionZ, 13.0f) *
                                2.0f)) %
      variantCount;
  const float companionScale =
      0.72f + coralDetailNoise(companionX, companionZ, 14.0f) * 0.22f;
  const float companionRotation =
      coralDetailNoise(companionX, companionZ, 15.0f) * glm::two_pi<float>();

  result.perVariantTransforms[companionVariant].push_back(buildCoralTransform(
      companionX, companionZ, companionRotation, companionScale,
      colonyStrength * 0.85f, seabed));
  ++result.totalCount;
}

inline void scatterCoralsAcrossScene(std::size_t variantCount,
                                     const SeabedParams &seabed,
                                     const std::vector<glm::vec2> &avoidPoints,
                                     CoralPlacementResult &result) {
  std::vector<glm::vec2> placed = avoidPoints;
  placed.reserve(128);

  for (float gridX = -kSceneHalfExtent; gridX <= kSceneHalfExtent;
       gridX += kScatterGridStep) {
    for (float gridZ = -kSceneHalfExtent; gridZ <= kSceneHalfExtent;
         gridZ += kScatterGridStep) {
      if (result.totalCount >= kMaxTotalCorals) {
        return;
      }

      const float jitterX =
          (coralDetailNoise(gridX, gridZ, 16.0f) - 0.5f) * kScatterGridStep *
          0.82f;
      const float jitterZ =
          (coralDetailNoise(gridX, gridZ, 17.0f) - 0.5f) * kScatterGridStep *
          0.82f;
      const glm::vec2 point(gridX + jitterX, gridZ + jitterZ);

      if (!isWithinScene(point.x, point.y)) {
        continue;
      }
      if (!isFarEnoughFromPoints(point, placed, kScatterMinSpacing)) {
        continue;
      }

      const float distFromShip =
          glm::length(point - glm::vec2(kShipAnchorX, kShipAnchorZ));
      if (distFromShip < kShipColonyNearMaxDist + 1.5f) {
        continue;
      }

      const std::size_t before = result.totalCount;
      addScatteredCoral(point.x, point.y, variantCount, seabed,
                        coralPatchNoise(point.x, point.y), result);
      if (result.totalCount > before) {
        placed.push_back(point);
      }
    }
  }
}

inline CoralPlacementResult
generateNoiseCoralPlacements(std::size_t variantCount,
                             const SeabedParams &seabed) {
  CoralPlacementResult result;
  result.perVariantTransforms.resize(variantCount);

  std::vector<std::pair<glm::vec2, float>> nearCandidates;
  nearCandidates.reserve(64);
  collectColonyCandidates(kShipColonyMinDist, kShipColonyNearMaxDist, 1.0f,
                          nearCandidates, seabed);

  std::vector<glm::vec2> colonyCenters;
  colonyCenters.reserve(static_cast<std::size_t>(kMaxNearReefColonies));
  pickColonyCenters(nearCandidates, kMaxNearReefColonies, colonyCenters);

  if (colonyCenters.empty()) {
    colonyCenters.emplace_back(kShipAnchorX + 8.0f, kShipAnchorZ + 4.0f);
    colonyCenters.emplace_back(kShipAnchorX - 7.0f, kShipAnchorZ - 5.0f);
  }

  for (const glm::vec2 &center : colonyCenters) {
    if (result.totalCount >= kMaxTotalCorals) {
      break;
    }
    addReefColony(center, variantCount, seabed, result);
  }

  scatterCoralsAcrossScene(variantCount, seabed, colonyCenters, result);
  return result;
}

inline glm::mat4 sharkPlacement(const SeabedParams &seabed) {
  const float x = 24.0f;
  const float z = -36.0f;
  const float seabedY = sampleSeabedWorldHeight(x, z, seabed);
  const float y = seabedY + 3.2f;

  glm::mat4 model(1.0f);
  model = glm::translate(model, glm::vec3(x, y, z));
  model = glm::rotate(model, glm::radians(-38.0f), glm::vec3(0.0f, 1.0f, 0.0f));
  model = glm::rotate(model, glm::radians(8.0f), glm::vec3(0.0f, 0.0f, 1.0f));
  model = glm::scale(model, glm::vec3(1.85f));
  return model;
}

inline glm::mat4 fishPlacementNoise(std::size_t index, const SeabedParams &seabed,
                                    float scaleMultiplier = 1.0f) {
  const float seedX = static_cast<float>(index) * 1.73f + 50.0f;
  const float seedZ = static_cast<float>(index) * 2.11f + 120.0f;
  const float seedY = static_cast<float>(index) * 0.97f + 210.0f;

  const float nx = seabedNoise.octave2D_01(seedX, seedZ, 3);
  const float nz = seabedNoise.octave2D_01(seedZ, seedX, 3);
  const float ny = seabedNoise.octave2D_01(seedY, seedX * 0.5f, 2);

  float x = -42.0f + nx * 84.0f;
  float z = -42.0f + nz * 38.0f;
  const float seabedY = sampleSeabedWorldHeight(x, z, seabed);
  float y = seabedY + 1.2f + ny * 7.5f;

  x = std::clamp(x, -42.0f, 42.0f);
  z = std::clamp(z, -42.0f, -2.0f);
  y = std::clamp(y, seabedY + 0.8f, 4.5f);

  const float rotation =
      seabedNoise.noise2D_01(seedX + 7.0f, seedZ + 3.0f) * glm::two_pi<float>();
  const float scale =
      (0.50f + seabedNoise.noise2D_01(seedY + 11.0f, seedX + 5.0f) * 0.35f) *
      scaleMultiplier;

  glm::mat4 model(1.0f);
  model = glm::translate(model, glm::vec3(x, y, z));
  model = glm::rotate(model, rotation, glm::vec3(0.0f, 1.0f, 0.0f));
  model = glm::scale(model, glm::vec3(scale));
  return model;
}

inline glm::mat4 sunkenShipPlacement(const SeabedParams &seabed) {
  const float x = kShipAnchorX;
  const float z = kShipAnchorZ;
  const float scale = 11.0f;
  const float seabedY = sampleSeabedWorldHeight(x, z, seabed);
  const float y = seabedY + 0.06f * scale;

  glm::mat4 model = glm::mat4(1.0f);
  model = glm::translate(model, glm::vec3(x, y, z));
  model = glm::rotate(model, glm::radians(-90.0f), glm::vec3(1.0f, 0.0f, 0.0f));
  model = glm::rotate(model, glm::radians(12.0f), glm::vec3(0.0f, 1.0f, 0.0f));
  model = glm::rotate(model, glm::radians(-20.0f), glm::vec3(0.0f, 0.0f, 1.0f));
  model = glm::rotate(model, glm::radians(6.0f), glm::vec3(0.0f, 1.0f, 0.0f));
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
