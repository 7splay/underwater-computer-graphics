#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <algorithm>
#include <cstddef>
#include <vector>

#include "coral_placement.hpp"
#include "seabed_height.hpp"

namespace {
constexpr float kClusterSeaweedSceneHalf = 50.0f;
constexpr std::size_t kMaxClusterSeaweed = 140;
constexpr float kClusterSeaweedMinSpacing = 4.5f;
constexpr float kClusterSeaweedGridStep = 6.0f;
constexpr float kClusterSeaweedPatchThreshold = 0.42f;
constexpr float kClusterPatchOffsetX = 401.0f;
constexpr float kClusterPatchOffsetZ = 173.0f;
}  // namespace

inline glm::mat4 buildSeaweedTransform(float x, float z, float yRotation,
                                       float widthScale, float heightScale,
                                       const SeabedParams &seabed) {
  const float seabedY = sampleSeabedWorldHeight(x, z, seabed);
  const float y = seabedY + 0.02f;

  glm::mat4 model = glm::mat4(1.0f);
  model = glm::translate(model, glm::vec3(x, y, z));
  model = glm::rotate(model, yRotation, glm::vec3(0.0f, 1.0f, 0.0f));
  model = glm::scale(model, glm::vec3(widthScale, heightScale, widthScale));
  return model;
}

inline float clusterSeaweedPatchNoise(float worldX, float worldZ) {
  return seabedNoise.octave2D_01((worldX + kClusterPatchOffsetX) / kPatchScale,
                                 (worldZ + kClusterPatchOffsetZ) / kPatchScale,
                                 3);
}

inline float clusterSeaweedDetailNoise(float worldX, float worldZ,
                                       float channel) {
  return seabedNoise.noise2D_01(worldX * 0.11f + channel * 29.0f,
                                worldZ * 0.14f + channel * 17.0f);
}

inline std::vector<glm::mat4>
generateClusterSeaweedPlacements(const SeabedParams &seabed,
                                 float density = 1.0f) {
  // scale the seaweed budget by density; 1.0 keeps the original count
  const std::size_t maxSeaweed = std::max<std::size_t>(
      1, static_cast<std::size_t>(kMaxClusterSeaweed * density));
  std::vector<glm::mat4> transforms;
  transforms.reserve(maxSeaweed);
  std::vector<glm::vec2> placed;
  placed.reserve(maxSeaweed);

  for (float gridX = -kClusterSeaweedSceneHalf; gridX <= kClusterSeaweedSceneHalf;
       gridX += kClusterSeaweedGridStep) {
    for (float gridZ = -kClusterSeaweedSceneHalf; gridZ <= kClusterSeaweedSceneHalf;
         gridZ += kClusterSeaweedGridStep) {
      if (transforms.size() >= maxSeaweed) {
        return transforms;
      }

      const float jitterX =
          (clusterSeaweedDetailNoise(gridX, gridZ, 1.0f) - 0.5f) *
          kClusterSeaweedGridStep * 0.78f;
      const float jitterZ =
          (clusterSeaweedDetailNoise(gridX, gridZ, 2.0f) - 0.5f) *
          kClusterSeaweedGridStep * 0.78f;
      const float worldX = gridX + jitterX;
      const float worldZ = gridZ + jitterZ;

      if (worldX < -kClusterSeaweedSceneHalf || worldX > kClusterSeaweedSceneHalf ||
          worldZ < -kClusterSeaweedSceneHalf || worldZ > kClusterSeaweedSceneHalf) {
        continue;
      }
      if (!isFarEnoughFromPoints(glm::vec2(worldX, worldZ), placed,
                                 kClusterSeaweedMinSpacing)) {
        continue;
      }
      if (clusterSeaweedPatchNoise(worldX, worldZ) < kClusterSeaweedPatchThreshold) {
        continue;
      }
      if (sampleSeabedSlope(worldX, worldZ, seabed) > 1.35f) {
        continue;
      }

      const float widthScale =
          0.82f + clusterSeaweedDetailNoise(worldX, worldZ, 3.0f) * 0.34f;
      const float heightScale =
          0.55f + clusterSeaweedDetailNoise(worldX, worldZ, 4.0f) * 0.42f;
      const float rotation =
          clusterSeaweedDetailNoise(worldX, worldZ, 5.0f) *
          glm::two_pi<float>();

      transforms.push_back(buildSeaweedTransform(
          worldX, worldZ, rotation, widthScale, heightScale, seabed));
      placed.emplace_back(worldX, worldZ);
    }
  }

  return transforms;
}
