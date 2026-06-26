# Underwater world interactive scene

**Authors:** Oleksandr Prybylov, Anna Vasylchenko (group 12)

## Chosen methods

- [x] **A07 — Instanced rendering with LOD:** corals, rocks, seaweed, and fish are drawn with instancing; each instance picks one of three LOD tiers based on camera distance. Corals ship three pre-decimated Blender meshes per variant (HIGH ~50-70k tris, MED ~3-5k, LOW ~500-700) for clean edge-collapse transitions instead of procedural triangle dropping. Seaweed and distant corals fall back to camera-facing billboards.
- [x] **B14 — Simple creature animation state machine:** each fish has `SWIM` / `FLEE` / `CHASE` states with transitions driven by flashlight ray hits and bait proximity.

## Mandatory methods

- [x] **Normal mapping** — sand seabed and coral surfaces, tangent-space TBN.
- [x] **PBR lighting** — metallic/roughness materials for sand, coral, rock, fish.
- [x] **Quaternion camera control** — first-person diver camera with quaternion mouse look, pitch clamp, WASD/vertical movement, and smoothed velocity.
- [x] **Shadow mapping** — flashlight spotlight depth map with 4-tap PCF and slope-scaled bias. Fish and corals cast shadows; seaweed is excluded from the depth pass since its alpha-cutout blades cast no visible shadow on sand.
- [x] **Parallel Transport Frames** — stable fish orientation along closed Catmull-Rom patrol loops around coral clusters.
- [x] **Underwater skybox/cubemap** — blue-green gradient, bright above, dark abyss below.

## Controls

| Input | Action |
| --- | --- |
| `W` `A` `S` `D` | move (forward / left / back / right) |
| Mouse | look around |
| `Space` / `Left Shift` | ascend / descend |
| `F` | toggle the diver flashlight (spotlight + shadows) |
| `E` | scare fish within 30 m of the camera into FLEE with jittered timers |
| Left mouse button | throw bait in the aim direction; landed bait attracts nearby fish (CHASE state) |
| `L` | (debug) raise ambient so the scene is inspectable without the spotlight |
| `P` | (debug) draw Catmull-Rom patrol loops as coloured line strips |
| `Esc` | quit |

## Interactions (B14 state machine)

| # | Interaction | Effect |
| --- | --- | --- |
| 1 | `F` toggle flashlight | spotlight + shadow map on/off |
| 2 | flashlight cone hits a fish | ray-sphere test within 18m → fish enters FLEE |
| 3 | Left mouse throws bait | projectile arc; once landed, nearby fish enter CHASE and seek the bait; consumed on contact |
| 4 | `E` scare-all | every fish within 30 m of the camera enters FLEE with jittered timers |

### Fish state machine

Each fish owns one of three states plus a per-state timer and a global cooldown.

```
                       flashlight cone hits fish (18 m, ray-sphere)
                ┌─────────────────────────────────────────────────────┐
                ▼                                                     │
            ┌────────┐   bait within 20 m    ┌────────┐               │
   ────────►│ SWIM   │──────────────────────►│ CHASE  │               │
            │ patrol │◄──────────────────────│ seek   │               │
            │ +boids │  bait eaten / timeout │ bait   │               │
            └────┬───┘                       └────────┘               │
                 │                                                    │
                 │  E key scare (30 m radius, jittered timers)        │
                 │  flashlight cone hit                               │
                 ▼                                                    │
            ┌────────┐   timer expires (~5 s, jittered)               │
            │ FLEE   │────────────────────────────────────────────────┘
            │ accel. │
            │ away   │
            └────────┘
```

States:

- **SWIM** — default. Each fish follows a closed Catmull-Rom patrol loop around its nearest coral cluster (PTF orientation along the spline) with boids separation/alignment/cohesion on top via a spatial grid.
- **FLEE** — triggered by a flashlight cone hit or the `E` scare-all. Fish accelerates away from the light source with a per-fish angular bias and slow wander so the school scatters rather than bolting in lockstep. Returns to SWIM after a jittered timer (~5 s) plus a short cooldown so it doesn't immediately re-trigger.
- **CHASE** — triggered when bait lands within 20 m. Overrides the flashlight response, so fish will swim through the cone to reach a worm. Fish dive in 3D toward the bait, park at nibble range with hysteresis (so they don't flicker on the boundary), and multiple feeders share the worm — it shrinks and vanishes when fully eaten.

Transition timing uses per-fish jittered delays and reaction windows so a school reacts as a wave, not a single frame.

## Screenshots

_Add screenshots here once the final demo build is ready._

## Build and run instructions
1. Clone the repository and enter root directory:
```bash
git clone https://github.com/7splay/underwater-computer-graphics.git
cd underwater-computer-graphics
```
2. Install Xcode Command Line Tools if not installed already (used for compiling C/C++ code):
```bash
xcode-select --install
```
3. Install brew if not installed already (used for downloading required libraries) - https://brew.sh/:
```bash
/bin/bash -c "$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/HEAD/install.sh)"
```
4. Install required libraries:
```bash
brew install cmake glfw glew assimp glm
```
5. Build the project:
```bash
mkdir -p build
cmake -S . -B build
cmake --build build
```
6. Run the compiled executable:
```bash
./build/underwater-computer-graphics
```

### Object density (optional)

The executable takes an optional density multiplier that scales how many corals, seaweed and fish are generated. `1.0` is the default scene; higher packs in more, lower thins it out (useful on slower machines):
```bash
./build/underwater-computer-graphics 0.5   # sparser, faster
./build/underwater-computer-graphics 2.0   # denser
```

The window title shows the current FPS. The app enables VSync (`glfwSwapInterval(1)`), so on a 60 Hz display it caps at a stable 60 FPS with no tearing.

## Asset pipeline

- `models/` — HIGH-tier meshes (full-detail). Fish, shark, worm, ship, and HIGH-tier corals live here.
- `models_med/` — pre-decimated MED-tier coral meshes (Blender edge-collapse at ~0.05 ratio, ~3-5k tris each).
- `models_low/` — pre-decimated LOW-tier coral meshes (~500-700 tris each). At distance, seaweed and far corals also fall back to camera-facing billboards.
- `img/` — textures and normal maps, resolved by path convention (e.g. `coral2.obj` → `coral2.png`).
- `shaders/` — GLSL vertex and fragment programs.

Coral meshes were re-exported from Blender with `DECIMATE` (COLLAPSE) to recover GPU-bound FPS: the original AI-generated exports came in at 250k-350k triangles each and dropped the scene to ~16 FPS. Pre-decimating to three tiers brought it to a stable 60 FPS with no visible quality loss on small props.