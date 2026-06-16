#pragma once

#include <glm/glm.hpp>

// Shared underwater look — keep in sync with shaders/model.frag fog block.
namespace underwater {
constexpr glm::vec3 kClearColor{0.04f, 0.14f, 0.24f};   // background / water tint
constexpr glm::vec3 kFogColor{0.04f, 0.16f, 0.26f};     // exponential fog
constexpr glm::vec3 kAmbientColor{0.04f, 0.10f, 0.14f}; // PBR ambient term
constexpr float kFogDensity = 0.028f;
constexpr float kFogMax = 0.92f;
}
