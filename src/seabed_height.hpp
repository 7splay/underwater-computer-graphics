#pragma once

#include "PerlinNoise.hpp"

#include <algorithm>
#include <cmath>

struct SeabedParams {
  int resX = 128;
  int resZ = 128;
  float gap = 1.0f;
  float amplitude = 5.0f;
  float smoothness = 50.0f;
  float modelY = -5.0f;
};

inline siv::PerlinNoise seabedNoise{42u};

inline float sampleSeabedWorldHeight(float worldX, float worldZ,
                                     const SeabedParams &params) {
  const float gridX = worldX / params.gap + params.resX * 0.5f;
  const float gridZ = worldZ / params.gap + params.resZ * 0.5f;

  const int x0 =
      std::clamp(static_cast<int>(std::floor(gridX)), 0, params.resX - 1);
  const int z0 =
      std::clamp(static_cast<int>(std::floor(gridZ)), 0, params.resZ - 1);
  const int x1 = std::min(x0 + 1, params.resX);
  const int z1 = std::min(z0 + 1, params.resZ);
  const float tx = gridX - static_cast<float>(x0);
  const float tz = gridZ - static_cast<float>(z0);

  const auto heightAt = [&](int x, int z) {
    return seabedNoise.octave2D_01(x / params.smoothness, z / params.smoothness,
                                   3);
  };

  const float h00 = heightAt(x0, z0);
  const float h10 = heightAt(x1, z0);
  const float h01 = heightAt(x0, z1);
  const float h11 = heightAt(x1, z1);
  const float h0 = h00 + (h10 - h00) * tx;
  const float h1 = h01 + (h11 - h01) * tx;
  const float h = h0 + (h1 - h0) * tz;

  return params.modelY + h * params.amplitude - params.amplitude * 0.5f;
}
