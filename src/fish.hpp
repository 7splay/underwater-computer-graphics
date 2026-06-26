#pragma once

#include <algorithm>
#include <cmath>

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

#include <cstddef>
#include <unordered_map>
#include <vector>

#include "flashlight.hpp"
#include "renderable.hpp"
#include "seabed_height.hpp"

// fish state machine (B14): SWIM cruise + FLEE escape + CHASE seek
enum class FishState { Swim, Flee, Chase };

// world bounds, derived from sand generation
constexpr float kWorldMinX = -60.0f, kWorldMaxX = 60.0f;
constexpr float kWorldMinZ = -60.0f, kWorldMaxZ = 60.0f;
constexpr float kWorldMaxY = 2.5f;
constexpr glm::vec3 kWorldCenter{0.0f, -1.0f, -5.0f};
constexpr float kBoundsMargin = 10.0f;

// state tuning
constexpr float kFleeDuration = 5.0f;
constexpr float kFleeJitter = 2.0f;
constexpr float kFleeCooldown = 1.5f;
constexpr float kScareRadius = 18.0f;
constexpr float kSwimSpeed = 1.4f;
constexpr float kFleeSpeed = 5.5f;
constexpr float kChaseSpeed = 3.2f;
constexpr float kTurnRate = 2.0f;
// desired-heading low-pass rate (1/s). higher = snappier turns, lower = more
// smoothing. kept above kTurnRate so the heading filter isn't the bottleneck
constexpr float kDirSmoothingRate = 6.0f;

// boids tuning
constexpr float kBoidNeighborRadius = 4.0f;
constexpr float kBoidSeparationRadius = 2.0f;
constexpr float kSeparationWeight = 2.4f;
constexpr float kAlignmentWeight = 0.5f;
constexpr float kCohesionWeight = 0.6f;
constexpr float kWanderWeight = 0.25f;
constexpr float kCellSize = kBoidNeighborRadius;

// bait / chase tuning
constexpr float kBaitLifespan = 8.0f;
constexpr float kBaitAttractRadius = 20.0f;
constexpr float kBaitConsumeRadius = 1.2f;
constexpr float kChaseDuration = 6.0f;
// how long a worm takes to be eaten by a single fish once one latches on
// additional feeders divide this time. the worm shrinks over the eat window
constexpr float kEatDuration = 2.0f;
// worm physics: gentle underwater descent (heavy but not free-falling) plus
// throw impulse so a click launches the worm out before it sinks
constexpr float kBaitGravity = 0.8f;
constexpr float kBaitDrag = 0.7f;
constexpr float kBaitThrowSpeed = 5.0f;
// cap on simultaneously active worms so spam clicking doesn't pile them up
constexpr std::size_t kBaitMax = 12;

// clearance above procedurally generated seabed so fish never clip through sand
constexpr float kSeabedClearance = 1.5f;
// single shared source-of-truth declared in seabed_height.hpp; keeps fish floor
// clamps, bait landing, coral/seaweed/shark placement, and the actual sand mesh
// in lockstep. previously each call site had its own default-constructed copy
// (128x128) that drifted from the 256x256 mesh after the LOD merge
inline const SeabedParams &kSeabedParams = kSceneSeabed;

struct Fish {
  Renderable *renderable = nullptr;
  glm::vec3 position = glm::vec3(0.0f);
  glm::vec3 velocity = glm::vec3(0.0f);
  glm::quat orientation = glm::quat(1.0f, 0.0f, 0.0f, 0.0f);
  // low-pass filtered desired heading. boids/neighbours flicker on/off the
  // radius every frame and cause the target to jitter left-right, which slerp
  // can't smooth fast enough -> visible rotation trembling. smoothing the dir
  // first removes that high-frequency noise before orientation tracking
  glm::vec3 smoothDir = glm::vec3(0.0f, 0.0f, -1.0f);
  float scale = 1.0f;
  FishState state = FishState::Swim;
  float stateTimer = 0.0f;
  float cooldown = 0.0f;
  // per-fish flee reaction delay so a swept flashlight doesn't panic every
  // hit fish on the same frame; small randomised window per fish
  float fleeDelay = 0.0f;
  std::size_t patrolIndex = 0;
  float patrolT = 0.0f;
  // latched flag for nibble-range hysteresis so fish don't flicker between
  // seek/park on the kBaitConsumeRadius boundary frame to frame
  bool nibbling = false;
};

inline Fish makeFish(Renderable *renderable, const glm::vec3 &position,
                     float scale, std::size_t patrolIndex = 0) {
  Fish fish;
  fish.renderable = renderable;
  fish.position = position;
  fish.scale = scale;
  fish.patrolIndex = patrolIndex;
  // pseudo-random reaction delay in [0, 0.35s]: when the cone sweeps over a
  // group, fish bolt one-by-one instead of all on the same frame
  static std::size_t sSeed = 0;
  fish.fleeDelay =
      0.35f * (0.5f + 0.5f * std::sin(static_cast<float>(++sSeed) * 2.13f));
  return fish;
}

// closed Catmull-Rom patrol loop. scout point advances along `t in [0,1)` and
// fish seek toward it. PTF orientation comes from the loop tangent
struct PatrolLoop {
  std::vector<glm::vec3> points;
  float speed = 0.05f;
  glm::vec3 center = glm::vec3(0.0f);
};

inline glm::vec3 sampleLoop(const PatrolLoop &loop, float t) {
  const std::size_t n = loop.points.size();
  if (n < 4) return loop.center;
  t = t - std::floor(t);
  const float f = t * static_cast<float>(n);
  const std::size_t i = static_cast<std::size_t>(f) % n;
  const std::size_t p0 = (i + n - 1) % n;
  const std::size_t p1 = i;
  const std::size_t p2 = (i + 1) % n;
  const std::size_t p3 = (i + 2) % n;
  const float u = f - static_cast<float>(i);
  const float u2 = u * u, u3 = u2 * u;
  return 0.5f * ((2.0f * loop.points[p1]) +
                 (-loop.points[p0] + loop.points[p2]) * u +
                 (2.0f * loop.points[p0] - 5.0f * loop.points[p1] +
                  4.0f * loop.points[p2] - loop.points[p3]) * u2 +
                 (-loop.points[p0] + 3.0f * loop.points[p1] -
                  3.0f * loop.points[p2] + loop.points[p3]) * u3);
}

inline glm::vec3 sampleLoopTangent(const PatrolLoop &loop, float t) {
  const float eps = 0.001f;
  const glm::vec3 d = sampleLoop(loop, t + eps) - sampleLoop(loop, t - eps);
  // degenerate loops (e.g. all control points equal) give a zero delta whose
  // normalize would be NaN -> propagate to fish heading -> visual twitch
  return glm::length(d) > 0.0001f ? glm::normalize(d) : glm::vec3(0.0f, 0.0f, -1.0f);
}

// yaw-only orientation: model forward is -X, fish never tilt or roll
// orientation from a world-space direction. yaw follows horizontal heading,
// pitch tilts nose up/down up to a clamp so fish can dive toward bait or
// rise along the patrol loop without rolling onto their side
inline glm::quat faceDirection(const glm::vec3 &worldDir, bool allowPitch = false) {
  glm::vec3 flat{worldDir.x, 0.0f, worldDir.z};
  const float flatLen = glm::length(flat);
  if (flatLen < 0.0001f) return glm::quat(1.0f, 0.0f, 0.0f, 0.0f);
  flat /= flatLen;
  const float yaw = std::atan2(flat.z, -flat.x);
  glm::quat q = glm::angleAxis(yaw, glm::vec3(0.0f, 1.0f, 0.0f));
  if (allowPitch) {
    // pitch = angle between full dir and its horizontal projection. clamp so
    // fish never go vertical (looks weird). negative = nose down
    const float fullLen = glm::length(worldDir);
    if (fullLen > 0.0001f) {
      const float rawPitch = std::asin(glm::clamp(worldDir.y / fullLen, -1.0f, 1.0f));
      const float pitch = glm::clamp(rawPitch, -0.5f, 0.5f);  // ~28 deg max
      // model pitches around its local X (right) axis after yaw is applied
      q = q * glm::angleAxis(-pitch, glm::vec3(1.0f, 0.0f, 0.0f));
    }
  }
  return q;
}

inline void syncFishModelMatrix(Fish &fish) {
  if (fish.renderable == nullptr) return;
  glm::mat4 model(1.0f);
  model = glm::translate(model, fish.position);
  model = model * glm::mat4_cast(fish.orientation);
  model = glm::scale(model, glm::vec3(fish.scale));
  fish.renderable->model = model;
}

inline glm::quat slerpToward(const glm::quat &current, const glm::quat &target,
                              float deltaTime, float turnRate) {
  return glm::slerp(current, target, std::clamp(turnRate * deltaTime, 0.0f, 1.0f));
}

// per-fish smooth wander so schools break lockstep without jitter
inline float wanderPhase(std::size_t i, float time) {
  const float freq = 0.4f + 0.07f * static_cast<float>(i % 5);
  const float phase = static_cast<float>(i) * 0.83f;
  return std::sin(time * freq + phase) * glm::two_pi<float>();
}

// uniform grid bucketing of fish positions for O(n) boids
struct FishSpatialGrid {
  std::unordered_map<long long, std::vector<std::size_t>> cells;

  static long long cellKey(int cx, int cz) {
    return (static_cast<long long>(cx) << 32) | static_cast<unsigned int>(cz);
  }

  void build(const std::vector<Fish> &fish) {
    cells.clear();
    for (std::size_t i = 0; i < fish.size(); ++i) {
      const int cx = static_cast<int>(std::floor(fish[i].position.x / kCellSize));
      const int cz = static_cast<int>(std::floor(fish[i].position.z / kCellSize));
      cells[cellKey(cx, cz)].push_back(i);
    }
  }

  glm::vec3 boidsForce(const std::vector<Fish> &fish, std::size_t i) const {
    const Fish &f = fish[i];
    const int cx = static_cast<int>(std::floor(f.position.x / kCellSize));
    const int cz = static_cast<int>(std::floor(f.position.z / kCellSize));

    glm::vec3 separation(0.0f), alignment(0.0f), cohesion(0.0f);
    int alignCount = 0, cohereCount = 0;

    for (int dx = -1; dx <= 1; ++dx) {
      for (int dz = -1; dz <= 1; ++dz) {
        auto it = cells.find(cellKey(cx + dx, cz + dz));
        if (it == cells.end()) continue;
        for (std::size_t j : it->second) {
          if (j == i) continue;
          glm::vec3 diff = fish[j].position - f.position;
          diff.y = 0.0f;
          const float dist = glm::length(diff);
          if (dist < kBoidNeighborRadius && dist > 0.001f) {
            const glm::vec3 dir = diff / dist;
            alignment += fish[j].velocity;
            cohesion += fish[j].position;
            ++alignCount;
            ++cohereCount;
            if (dist < kBoidSeparationRadius)
              separation -= dir * (1.0f - dist / kBoidSeparationRadius);
          }
        }
      }
    }

    glm::vec3 force(0.0f);
    if (glm::length(separation) > 0.001f)
      force += glm::normalize(separation) * kSeparationWeight;
    if (alignCount > 0) {
      glm::vec3 avg = alignment / static_cast<float>(alignCount);
      avg.y = 0.0f;
      if (glm::length(avg) > 0.001f)
        force += glm::normalize(avg) * kAlignmentWeight;
    }
    if (cohereCount > 0) {
      glm::vec3 toCenter = cohesion / static_cast<float>(cohereCount) - f.position;
      toCenter.y = 0.0f;
      if (glm::length(toCenter) > 0.001f)
        force += glm::normalize(toCenter) * kCohesionWeight;
    }
    return force;
  }
};

// bait projectile: thrown from camera, lands and attracts nearby fish
struct Bait {
  glm::vec3 position = glm::vec3(0.0f);
  glm::vec3 velocity = glm::vec3(0.0f);
  float lifetime = 0.0f;
  bool landed = false;
  bool consumed = false;
  // eat progress in [0,1]. advances when fish are within kBaitConsumeRadius,
  // faster with more feeders. the worm shrinks and vanishes when progress = 1
  float eatProgress = 0.0f;
  // facing yaw, picked at landing so each worm settles in a natural direction
  float restYaw = 0.0f;
};

inline std::vector<Bait> baits;

inline void updateBait(std::vector<Bait> &baits, float deltaTime,
                       const std::vector<Fish> &fish) {
  for (Bait &b : baits) {
    if (b.consumed) continue;
    b.lifetime += deltaTime;
    if (!b.landed) {
      // gentle drift: low drag and weak gravity so the worm floats down softly
      // like it's heavier than water but not in free-fall
      const glm::vec3 drag = -b.velocity * kBaitDrag;
      b.velocity += (glm::vec3(0.0f, -kBaitGravity, 0.0f) + drag) * deltaTime;
      b.position += b.velocity * deltaTime;
      // rest on the actual procedural seabed so the worm lies on the sand
      const float groundY =
          sampleSeabedWorldHeight(b.position.x, b.position.z, kSeabedParams) +
          kSeabedClearance;
      if (b.position.y <= groundY) {
        b.position.y = groundY;
        b.velocity = glm::vec3(0.0f);
        b.landed = true;
        b.restYaw = static_cast<float>(b.lifetime) * 1.7f;
      }
    }
    // count fish within kBaitConsumeRadius; each feeder advances eatProgress
    // at 1/kEatDuration per second, so two fish finish in half the time
    if (b.landed) {
      int feeders = 0;
      for (const Fish &f : fish) {
        if (glm::length(f.position - b.position) < kBaitConsumeRadius) ++feeders;
      }
      if (feeders > 0) {
        b.eatProgress +=
            (static_cast<float>(feeders) / kEatDuration) * deltaTime;
        if (b.eatProgress >= 1.0f) b.consumed = true;
      }
    }
    if (b.lifetime > kBaitLifespan) b.consumed = true;
  }
  // remove consumed baits immediately so the worm vanishes once eaten/expired
  baits.erase(std::remove_if(baits.begin(), baits.end(),
                              [](const Bait &b) { return b.consumed; }),
              baits.end());
}

inline void throwBait(const glm::vec3 &origin, const glm::vec3 &aimDir) {
  // soft cap: keep newest N baits to avoid unbounded growth under spam clicking
  if (baits.size() >= kBaitMax) baits.erase(baits.begin());
  // per-throw varied restYaw so each worm lies facing a different direction
  const float r = static_cast<float>(baits.size()) * 0.618f;
  Bait b;
  b.position = origin;
  // softer throw than before so the worm drifts out and sinks gently
  b.velocity = aimDir * kBaitThrowSpeed + glm::vec3(0.0f, 1.0f, 0.0f);
  b.restYaw = r * 6.28f;
  baits.push_back(b);
}

// find nearest active bait within attract radius; returns -1 if none
// uses full 3D distance so airborne worms can be intercepted mid-descent
// worms being eaten are still valid targets: multiple fish can crowd and
// feed together, which actually speeds up consumption
inline int findNearestBait(const glm::vec3 &pos) {
  float bestDist = kBaitAttractRadius;
  int bestIdx = -1;
  for (std::size_t i = 0; i < baits.size(); ++i) {
    const Bait &b = baits[i];
    if (b.consumed) continue;
    const float d = glm::length(pos - b.position);
    if (d < bestDist) {
      bestDist = d;
      bestIdx = static_cast<int>(i);
    }
  }
  return bestIdx;
}

inline void updateFish(std::vector<Fish> &fish, float deltaTime, float time,
                       const std::vector<PatrolLoop> &patrols,
                       bool flashlightOn, const glm::vec3 &lightPos) {
  static FishSpatialGrid grid;
  grid.build(fish);

  for (std::size_t i = 0; i < fish.size(); ++i) {
    Fish &f = fish[i];
    if (f.cooldown > 0.0f) f.cooldown -= deltaTime;

    // only advance the scout while cruising so fleeing/chasing fish don't let
    // the patrol target lap far ahead of them and snap back when they return
    if (f.state == FishState::Swim && f.patrolIndex < patrols.size())
      f.patrolT += patrols[f.patrolIndex].speed * deltaTime;

    // state transitions
    const int baitIdx = findNearestBait(f.position);
    const bool hasBait = baitIdx >= 0;
    const glm::vec3 baitPos = hasBait ? baits[baitIdx].position : glm::vec3(0.0f);

    if (f.state == FishState::Flee) {
      if (flashlightOn && flashlight::hitsPoint(f.position, kScareRadius))
        f.stateTimer = kFleeDuration;
      f.stateTimer -= deltaTime;
      if (f.stateTimer <= 0.0f) {
        f.state = FishState::Swim;
        f.cooldown = kFleeCooldown;
      }
    } else if (f.state == FishState::Chase) {
      // no bait left -> give up immediately instead of running SWIM at chase speed
      if (!hasBait) {
        f.state = FishState::Swim;
        f.nibbling = false;
        f.cooldown = kFleeCooldown;
      } else {
        // feeding happens via proximity in updateBait (eatProgress). while in
        // range the fish just parks at the worm so it visibly nibbles; the
        // chase timer only counts down outside the consume radius so a fish
        // that reaches the worm won't get yanked back to SWIM mid-meal
        const float baitDist = glm::length(baitPos - f.position);
        if (baitDist >= kBaitConsumeRadius) {
          f.stateTimer -= deltaTime;
        }
        if (f.stateTimer <= 0.0f) {
          f.state = FishState::Swim;
          f.nibbling = false;
          f.cooldown = kFleeCooldown;
        }
      }
    } else if (f.state == FishState::Swim) {
      // bait priority: a nearby worm overrides everything, fish will swim
      // through the flashlight cone to reach it instead of fleeing
      if (hasBait) {
        f.state = FishState::Chase;
        f.nibbling = false;
        f.stateTimer = kChaseDuration;
      } else if (flashlightOn && f.cooldown <= 0.0f &&
                 flashlight::hitsPoint(f.position, kScareRadius)) {
        // staggered reaction: each fish only checks the cone during its own
        // short time slot, so when the light sweeps a school they bolt one
        // after another instead of all on the same frame
        constexpr float kReactionWindow = 0.4f;
        const float slot = std::fmod(time + f.fleeDelay, kReactionWindow);
        if (slot < deltaTime * 1.5f) {
          f.state = FishState::Flee;
          f.stateTimer =
              kFleeDuration +
              (std::sin(i * 7.13f) + std::cos(i * 3.77f)) * 0.5f * kFleeJitter;
        }
      }
    }

    // desired heading per state
    glm::vec3 dir(0.0f, 0.0f, -1.0f);
    if (f.state == FishState::Flee) {
      // base flee direction is radially away from the light, but add a per-fish
      // angular bias and a slow wandering turn so the school scatters in
      // different directions and curves over time instead of beelining in
      // lockstep on a single straight line
      glm::vec3 away = f.position - lightPos;
      away.y = 0.0f;
      if (glm::length(away) > 0.001f) {
        away = glm::normalize(away);
        // per-fish fixed scatter bias + time-driven wander so directions diverge
        const float biasDeg = (std::sin(i * 2.37f) + std::cos(i * 1.81f)) * 35.0f;
        const float wanderDeg = std::sin(time * 0.9f + i * 0.7f) * 12.0f;
        const float angle = glm::radians(biasDeg + wanderDeg);
        const float ca = std::cos(angle);
        const float sa = std::sin(angle);
        dir = glm::vec3(away.x * ca - away.z * sa, 0.0f,
                        away.x * sa + away.z * ca);
      }
      // soften with a little boids so fleeing fish don't tunnel through each
      // other; keeps the school readable as a fleeing group
      dir += grid.boidsForce(fish, i) * 0.3f;
    } else if (f.state == FishState::Chase) {
      if (hasBait) {
        const float dist = glm::length(baitPos - f.position);
        // hysteresis: enter nibble range at kBaitConsumeRadius, but only
        // leave it once you're outside the wider kBaitConsumeRadius + slack
        // without this, fish straddling the boundary flicker in/out of the
        // parked branch every frame -> trembling right at the bait
        constexpr float kNibbleSlack = 0.6f;
        const bool inNibble =
            f.nibbling ? dist < kBaitConsumeRadius + kNibbleSlack
                       : dist < kBaitConsumeRadius;
        f.nibbling = inNibble;
        if (inNibble) {
          // close enough: stop and feed. dir = 0 parks the fish (held by
          // smoothDir logic). no orbit, no boids, no twitch
          dir = glm::vec3(0.0f);
        } else {
          // full 3D seek toward the worm with light separation so fish
          // don't tunnel through each other on the approach
          glm::vec3 toBait = baitPos - f.position;
          dir = glm::length(toBait) > 0.001f ? glm::normalize(toBait) : dir;
          dir += grid.boidsForce(fish, i) * 0.4f;
        }
      }
    } else {
      // SWIM: scout on patrol loop + boids + wander
      const glm::vec3 boidsForce = grid.boidsForce(fish, i);
      const float wanderAngle = wanderPhase(i, time);
      glm::vec3 cruise(0.0f, 0.0f, -1.0f);
      if (f.patrolIndex < patrols.size()) {
        const PatrolLoop &loop = patrols[f.patrolIndex];
        // keep Y in the seek direction so fish actually ride the patrol loop's
        // vertical variation instead of cruising on a flat plane
        glm::vec3 scout = sampleLoop(loop, f.patrolT);
        // never let the scout dip below the seabed at this XZ, even if the
        // interpolated Catmull-Rom point does - prevents dive-bounce tweaking
        const float scoutFloorY =
            sampleSeabedWorldHeight(scout.x, scout.z, kSeabedParams) +
            kSeabedClearance;
        scout.y = std::max(scout.y, scoutFloorY);
        glm::vec3 toScout = scout - f.position;
        if (glm::length(toScout) > 0.5f) {
          cruise = glm::normalize(toScout);
        } else {
          // scout is essentially on top of the fish: follow the tangent instead
          const glm::vec3 tan = sampleLoopTangent(loop, f.patrolT);
          cruise = glm::length(tan) > 0.001f ? tan : cruise;
        }
      }
      dir = cruise * 2.0f + boidsForce +
            glm::vec3(std::sin(wanderAngle), 0.0f, std::cos(wanderAngle)) * kWanderWeight;

      glm::vec3 toCenter = kWorldCenter - f.position;
      toCenter.y = 0.0f;
      const float edge = std::min({f.position.x - kWorldMinX, kWorldMaxX - f.position.x,
                                    f.position.z - kWorldMinZ, kWorldMaxZ - f.position.z});
      if (edge < kBoundsMargin && glm::length(toCenter) > 0.001f) {
        const float pull = 1.0f - (edge / kBoundsMargin);
        dir = glm::mix(dir, glm::normalize(toCenter), pull);
      }
    }

    // a zero dir means the fish is parked (feeding on a worm). skip the
    // (0,0,-1) fallback and the normalization so velocity is exactly zero
    const bool parked = glm::length(dir) < 0.001f;
    if (!parked) {
      dir = glm::normalize(dir);
      // low-pass the desired heading. boids/seek contributions oscillate as
      // neighbours enter/exit the radius, and applying them raw makes the
      // target heading jitter left-right frame to frame, which slerp can't
      // absorb fast enough -> rotation trembling. smooth before tracking
      const float kDirSmooth = 1.0f - std::exp(-kDirSmoothingRate * deltaTime);
      f.smoothDir = glm::normalize(
          glm::mix(f.smoothDir, dir, kDirSmooth));
    }

    // orientation always tracks the actual movement direction. in SWIM the
    // patrol loop is the dominant term in `dir`, so fish effectively follow the
    // Catmull-Rom tangent (PTF-driven path) without ever desyncing from heading
    // pitch only when swimming or chasing so fleeing fish stay level. SWIM
    // follows the patrol loop's vertical curve, CHASE pitches to dive on bait
    // parked fish hold their last heading so they don't snap
    if (!parked) {
      const bool allowPitch = f.state != FishState::Flee;
      const glm::quat target = faceDirection(f.smoothDir, allowPitch);
      f.orientation = slerpToward(f.orientation, target, deltaTime, kTurnRate);
    }

    float speed = kSwimSpeed;
    if (f.state == FishState::Flee) {
      speed = kFleeSpeed;
    } else if (f.state == FishState::Chase) {
      // parked at the worm = full stop; otherwise full chase speed
      speed = parked ? 0.0f : kChaseSpeed;
    }
    f.velocity = parked ? glm::vec3(0.0f) : f.smoothDir * speed;
    f.position += f.velocity * deltaTime;

    f.position.x = std::clamp(f.position.x, kWorldMinX, kWorldMaxX);
    f.position.z = std::clamp(f.position.z, kWorldMinZ, kWorldMaxZ);
    // soft seabed: push the fish up if it dips below the dune surface
    const float seabedY =
        sampleSeabedWorldHeight(f.position.x, f.position.z, kSeabedParams) +
        kSeabedClearance;
    f.position.y = std::clamp(f.position.y, seabedY, kWorldMaxY);

    syncFishModelMatrix(f);
  }
}

// E-key interaction: every fish within scareRadius of origin panics into FLEE
// radius is larger than the flashlight cone so E reads as a wider "shockwave"
constexpr float kScareAllRadius = 30.0f;
inline void scareAllFish(std::vector<Fish> &fish, const glm::vec3 &origin) {
  for (std::size_t i = 0; i < fish.size(); ++i) {
    Fish &f = fish[i];
    if (f.state == FishState::Flee) continue;
    const float d = glm::length(glm::vec2(f.position.x - origin.x,
                                          f.position.z - origin.z));
    if (d > kScareAllRadius) continue;
    f.state = FishState::Flee;
    f.stateTimer =
        kFleeDuration +
        (std::sin(i * 7.13f) + std::cos(i * 3.77f)) * 0.5f * kFleeJitter;
  }
}
