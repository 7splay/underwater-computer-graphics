#pragma once

#include <glm/glm.hpp>

// Shared underwater look — keep in sync with shaders/model.frag fog block.
namespace underwater {
constexpr glm::vec3 kClearColor{0.03f, 0.10f, 0.18f};   // background / water tint
constexpr glm::vec3 kFogColor{0.03f, 0.11f, 0.19f};     // exponential fog
constexpr glm::vec3 kAmbientColor{0.025f, 0.06f, 0.09f}; // PBR ambient term
constexpr float kFogDensity = 0.028f;
constexpr float kFogMax = 0.92f;

// Visible sun direction (normalized in shaders). Used by the procedural skybox
// to place a soft glow; shared so the spot in the sky matches the scene lighting.
constexpr glm::vec3 kSunDirection{0.35f, 0.55f, -0.45f};
constexpr glm::vec3 kSunColor{1.0f, 0.95f, 0.82f};
}
