# Underwater world interactive scene

**Authors:** Oleksandr Prybylov, Anna Vasylchenko (group 12)

## Chosen methods

- [ ] **A07 — Instanced rendering with LOD:** corals, rocks, seaweed, and fish are drawn with instancing; each instance picks one of three LOD tiers (HIGH / MED / LOW billboard) based on camera distance.
- [ ] **B14 — Simple creature animation state machine:** each fish has `SWIM` / `FLEE` / `CHASE` states with transitions driven by flashlight ray hits and bait proximity.

## Mandatory methods

- [x] **Normal mapping** — sand seabed and coral surfaces, tangent-space TBN.
- [x] **PBR lighting** — metallic/roughness materials for sand, coral, rock, fish.
- [x] **Quaternion camera control** — first-person diver camera with quaternion mouse look, pitch clamp, WASD/vertical movement, and smoothed velocity.
- [ ] **Shadow mapping** — flashlight spotlight depth map with PCF.
- [ ] **Parallel Transport Frames** — stable fish orientation along Catmull-Rom patrol splines.
- [x] **Underwater skybox/cubemap** — blue-green gradient, bright above, dark abyss below.

## Build and run instructions
### MacOS
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