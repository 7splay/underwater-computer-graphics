#include <GL/glew.h>
#include <GLFW/glfw3.h>

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <cmath>
#include <cstdio>
#include <vector>

#include "camera.hpp"
#include "fish.hpp"
#include "flashlight.hpp"
#include "goggles.hpp"
#include "instancing.hpp"
#include "models.hpp"
#include "lod_system.hpp"
#include "renderer.hpp"
#include "scene_loader.hpp"
#include "seabed.hpp"
#include "shadow_map.hpp"
#include "skybox.hpp"
#include "underwater_atmosphere.hpp"

int width = 800;
int height = 600;

ModelProgram modelProgram{};
ShadowProgram shadowProgram{};
ShadowMap shadowMap{};
Goggles goggles{};
LodScene lodScene{};
std::vector<Renderable> fishRenderables;
std::vector<Fish> fish;
std::vector<PatrolLoop> patrols;
Renderable sandRenderable;
Renderable baitMesh;  // worm model drawn at each active bait position

  // object-density entry parameter (set from argv in main): scales how many
  // corals, seaweed and fish are generated. 1.0 = default scene
float objectDensity = 1.0f;

// debug full-scene light toggle (L key): flattens ambient so the scene can be
// inspected without the spotlight. independent of flashlight::enabled
bool debugLight = false;
// debug patrol loop drawing (P key): renders the Catmull-Rom loops as line
// strips so you can see the paths fish are following
bool debugPatrols = false;
GLuint debugLineProgram = 0;
GLint debugLineViewLoc = -1;
GLint debugLineProjLoc = -1;
GLint debugLineModelLoc = -1;
GLint debugLineColorLoc = -1;

GLuint skyboxProgram = 0;
Renderable skyboxRenderable;
GLuint skyboxTexture = 0;
GLint skyboxViewLoc = -1;
GLint skyboxProjLoc = -1;
GLint skyboxSamplerLoc = -1;
GLint skyboxSunDirLoc = -1;
GLint skyboxSunColorLoc = -1;
GLint skyboxFogColorLoc = -1;

float deltaTime = 0.0f;
float lastTime = 0.0f;

float cameraSpeed = 5.0f;

float mouseSensitivity = 0.003f;
bool firstMouse = true;
void processMouse(GLFWwindow *, double xPos, double yPos);
float lastX = floor(width / 2.0f);
float lastY = floor(height / 2.0f);

float sandDensity=1.0f;
float sandAmplitude=5.0f;
float sandSmoothness=50.0f;
glm::mat4 sandModel=glm::translate(glm::mat4(1.0f), glm::vec3(.0f,-5.0f,.0f));
// sand params mirror kSceneSeabed (in seabed_height.hpp) so that any height
// query against the seabed returns the same surface the mesh was generated with

glm::vec3 lastLodCameraPos(0.0f);
bool lodCameraInitialized = false;
constexpr float kLodUpdateMoveThreshold = 1.5f;

void framebuffer_size_callback(GLFWwindow *, int width, int height) {
  glViewport(0, 0, width, height);
}

void initModel(GLFWwindow *window) {
  glEnable(GL_DEPTH_TEST);
  glEnable(GL_TEXTURE_CUBE_MAP_SEAMLESS);  // blends cubemap across face edges

  modelProgram = createModelProgram();
  shadowProgram = createShadowProgram();
  shadowMap = createShadowMap(512, 512);
  goggles = createGoggles();

  SceneLoadResult sceneLoad = loadSceneModels(
      [window]() {
        if (window != nullptr) {
          glfwPollEvents();
        }
      },
      objectDensity);
  lodScene = std::move(sceneLoad.lod);
  fishRenderables = std::move(sceneLoad.fishRenderables);

  // build closed patrol loops around coral clusters: 6 control points each,
  // arranged as a wobbly circle above the anchor at cruise depth. loop Y is
  // clamped above the actual seabed at each control point so scouts never
  // dip underground and trigger seabed-clamp bouncing
  // patrol loops: scatter anchors on a coarse jittered grid across the whole
  // playable area ([-45, 45] x [-45, 45]) instead of reusing coral anchors.
  // coral anchors now sit near the ship at one corner of the map, so reusing
  // them stacked every patrol on top of each other. using our own anchors
  // also lets us pick a fixed count regardless of how many corals exist
  patrols.clear();
  constexpr std::size_t kPatrolGrid = 4;       // 4x4 = 16 candidate cells
  constexpr float kPatrolExtent = 45.0f;       // stay inside the +/-50 camera box
  constexpr float kPatrolCell =
      (2.0f * kPatrolExtent) / static_cast<float>(kPatrolGrid - 1);
  for (std::size_t iz = 0; iz < kPatrolGrid; ++iz) {
    for (std::size_t ix = 0; ix < kPatrolGrid; ++ix) {
      // deterministic jitter so the layout is stable across frames
      const float jx = std::sin(ix * 12.9898f + iz * 78.233f) * 43758.5453f;
      const float jz = std::cos(ix * 39.3468f + iz * 11.135f) * 12543.987f;
      const float jitterX = (jx - std::floor(jx) - 0.5f) * kPatrolCell * 0.8f;
      const float jitterZ = (jz - std::floor(jz) - 0.5f) * kPatrolCell * 0.8f;
      const float anchorX =
          -kPatrolExtent + static_cast<float>(ix) * kPatrolCell + jitterX;
      const float anchorZ =
          -kPatrolExtent + static_cast<float>(iz) * kPatrolCell + jitterZ;
      const float anchorSeabedY =
          sampleSeabedWorldHeight(anchorX, anchorZ, kSeabedParams);

      PatrolLoop loop;
      loop.center = glm::vec3(anchorX, anchorSeabedY, anchorZ);
      loop.speed = 0.04f;
      const std::size_t kPoints = 6;
      for (std::size_t k = 0; k < kPoints; ++k) {
        const float a = static_cast<float>(k) / static_cast<float>(kPoints) * glm::two_pi<float>();
        const float r = 6.0f + 1.5f * std::sin(a * 2.0f + anchorX);
        const float px = anchorX + std::cos(a) * r;
        const float pz = anchorZ + std::sin(a) * r;
        const float seabedY = sampleSeabedWorldHeight(px, pz, kSeabedParams);
        // cruise 2-4m above the seabed with smooth variation, never below
        const float variation = 2.0f * std::sin(a * 3.0f + anchorZ);
        const float py = std::max(seabedY + 2.0f, anchorSeabedY + 1.0f + variation);
        loop.points.push_back(glm::vec3(px, py, pz));
      }
      patrols.push_back(loop);
    }
  }

  // build fish entities. patrol assignment is re-derived here against the
  // actual patrols vector (built above) instead of trusting spawn.patrolIndex,
  // because spawn.patrolIndex was computed in scene_loader against coral
  // anchors which no longer match the patrol anchors
  for (const FishSpawn &spawn : sceneLoad.fishSpawns) {
    if (spawn.renderableIndex >= fishRenderables.size()) continue;
    std::size_t patrol = 0;
    if (!patrols.empty()) {
      float bestDist = std::numeric_limits<float>::max();
      for (std::size_t p = 0; p < patrols.size(); ++p) {
        const glm::vec2 d2(spawn.position.x - patrols[p].center.x,
                           spawn.position.z - patrols[p].center.z);
        const float d = glm::dot(d2, d2);
        if (d < bestDist) {
          bestDist = d;
          patrol = p;
        }
      }
    }
    fish.push_back(makeFish(&fishRenderables[spawn.renderableIndex],
                            spawn.position, spawn.scale, patrol));
    fish.back().patrolT =
        std::fmod(static_cast<float>(spawn.renderableIndex) * 0.17f, 1.0f);
    syncFishModelMatrix(fish.back());
  }
  sandRenderable = generateSand(kSceneSeabed.resX, kSceneSeabed.resZ,
                                kSceneSeabed.gap, kSceneSeabed.amplitude,
                                kSceneSeabed.smoothness,
                                glm::translate(glm::mat4(1.0f),
                                               glm::vec3(0.0f,
                                                         kSceneSeabed.modelY,
                                                         0.0f)));

  skyboxProgram = createProgram("shaders/skybox.vert", "shaders/skybox.frag");
  skyboxRenderable = makeSkybox();
  skyboxTexture = generateSkyboxCubemap();
  // debug line shader for patrol loop visualisation (toggled with P)
  debugLineProgram =
      createProgram("shaders/debug_line.vert", "shaders/debug_line.frag");
  debugLineViewLoc = glGetUniformLocation(debugLineProgram, "uView");
  debugLineProjLoc = glGetUniformLocation(debugLineProgram, "uProjection");
  debugLineModelLoc = glGetUniformLocation(debugLineProgram, "uModel");
  debugLineColorLoc = glGetUniformLocation(debugLineProgram, "uColor");
  // bait mesh: worm model drawn at each active bait position. loads through
  // loadBaitMesh so it shares the same "Loading model:" log + progress callback
  // as the rest of the scene. picking the largest sub-mesh discards Blender's
  // debug junk (3D text labels + a plane) baked into worm.obj
  baitMesh = loadBaitMesh("worm.obj", []() {});
  skyboxViewLoc = glGetUniformLocation(skyboxProgram, "uView");
  skyboxProjLoc = glGetUniformLocation(skyboxProgram, "uProjection");
  skyboxSamplerLoc = glGetUniformLocation(skyboxProgram, "uSkybox");
  skyboxSunDirLoc = glGetUniformLocation(skyboxProgram, "uSunDirection");
  skyboxSunColorLoc = glGetUniformLocation(skyboxProgram, "uSunColor");
  glUseProgram(skyboxProgram);
  if (skyboxSamplerLoc >= 0) {
    // dedicated unit 6 for the cubemap: keeping it off unit 0 avoids aliasing a
    // samplerCube and the model sampler2D on the same unit, which made macOS
    // log "unit 0 ... bound to sampler type (Float) ... unloadable"
    glUniform1i(skyboxSamplerLoc, 6);
  }
  if (skyboxSunDirLoc >= 0) {
    glUniform3fv(skyboxSunDirLoc, 1, glm::value_ptr(underwater::kSunDirection));
  }
  if (skyboxSunColorLoc >= 0) {
    glUniform3fv(skyboxSunColorLoc, 1, glm::value_ptr(underwater::kSunColor));
  }
  skyboxFogColorLoc = glGetUniformLocation(skyboxProgram, "uFogColor");
}

void init(GLFWwindow *window) {
  glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
#ifdef __APPLE__
  glViewport(0, 0, width * 2, height * 2);
#else
  glViewport(0, 0, width, height);
#endif

  initCamera();
  initModel(window);
  glfwSetCursorPosCallback(window, processMouse);
  glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
}

void renderScene(GLFWwindow *window) {
  // Water background color - see underwater_atmosphere.hpp.
  glClearColor(underwater::kClearColor.r, underwater::kClearColor.g,
               underwater::kClearColor.b, 1.0f);
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

  glfwGetFramebufferSize(window, &width, &height);
  if (height == 0) {
    height = 1;
  }

  glm::mat4 view = getViewMatrix();
  glm::mat4 projection = glm::perspective(
      glm::radians(48.0f),
      static_cast<float>(width) / static_cast<float>(height), 0.1f, 180.0f);

  const float sceneTime = static_cast<float>(glfwGetTime());

  if (!lodCameraInitialized ||
      glm::length(cameraPosition - lastLodCameraPos) >= kLodUpdateMoveThreshold) {
    updateLodScene(lodScene, cameraPosition);
    lastLodCameraPos = cameraPosition;
    lodCameraInitialized = true;
  }

  // shadow pass uses the unbiased VP, lit pass uses the biased one
  const glm::mat4 lightViewProj =
      flashlight::viewProjectionMatrix(1.0f);
  const glm::mat4 lightSpaceBiased =
      flashlight::biasedLightSpaceMatrix(1.0f);
  const bool flashlightOn = flashlight::enabled;
  if (flashlightOn) {
    bindShadowMapForWriting(shadowMap);
    glUseProgram(shadowProgram.id);
    for (const Renderable &renderable : lodScene.staticRenderables) {
      drawRenderableShadow(shadowProgram, renderable, lightViewProj);
    }
    for (LodInstancedGroup &group : lodScene.groups) {
      // shadow pass must render EVERY instance of a caster, not just the
      // high-tier bucket. updateLodScene buckets by player distance, so when
      // a coral is far from the player it sits in med/low and group.high has
      // 0 instances, which silently dropped it from the shadow map. push all
      // source matrices into the high-tier VBO before drawing shadows
      if (group.castsShadow && !group.sourceMatrices.empty()) {
        uploadInstanceMatrices(group.high, group.sourceMatrices);
        drawRenderableShadow(shadowProgram, group.high, lightViewProj);
        for (Renderable &extra : group.extraHigh) {
          drawRenderableShadow(shadowProgram, extra, lightViewProj);
        }
      }
    }
    drawRenderableShadow(shadowProgram, sandRenderable, lightViewProj);
    // fish cast shadows too; their model matrices were updated this frame by
    // updateFish before renderScene, so the depth pass matches their pose
    for (const Renderable &renderable : fishRenderables) {
      drawRenderableShadow(shadowProgram, renderable, lightViewProj);
    }
    unbindShadowMap(0, 0, width, height);
  }

  glUseProgram(modelProgram.id);

  if (modelProgram.view >= 0) {
    glUniformMatrix4fv(modelProgram.view, 1, GL_FALSE, glm::value_ptr(view));
  }
  if (modelProgram.projection >= 0) {
    glUniformMatrix4fv(modelProgram.projection, 1, GL_FALSE,
                       glm::value_ptr(projection));
  }
  if (modelProgram.cameraPos >= 0) {
    glUniform3fv(modelProgram.cameraPos, 1, glm::value_ptr(cameraPosition));
  }

  // flashlight uniforms; shadow texture is bound to unit 4 below
  if (modelProgram.lightSpace >= 0) {
    glUniformMatrix4fv(modelProgram.lightSpace, 1, GL_FALSE,
                       glm::value_ptr(lightSpaceBiased));
  }
  const glm::vec3 flashPos = flashlight::position();
  const glm::vec3 flashDir = flashlight::direction();
  if (modelProgram.flashPos >= 0) {
    glUniform3fv(modelProgram.flashPos, 1, glm::value_ptr(flashPos));
  }
  if (modelProgram.flashDir >= 0) {
    glUniform3fv(modelProgram.flashDir, 1, glm::value_ptr(flashDir));
  }
  if (modelProgram.flashRange >= 0) {
    glUniform1f(modelProgram.flashRange, flashlight::kRange);
  }
  // inner cone = full intensity, fades to zero at the outer cone
  const float outerDeg = flashlight::kConeHalfAngleDeg;
  const float innerDeg = outerDeg * 0.7f;
  if (modelProgram.flashCosInner >= 0) {
    glUniform1f(modelProgram.flashCosInner,
                std::cos(glm::radians(innerDeg)));
  }
  if (modelProgram.flashCosOuter >= 0) {
    glUniform1f(modelProgram.flashCosOuter,
                std::cos(glm::radians(outerDeg)));
  }
  if (modelProgram.flashColor >= 0) {
    glUniform3f(modelProgram.flashColor, 1.0f, 0.96f, 0.85f);
  }
  if (modelProgram.flashIntensity >= 0) {
    glUniform1f(modelProgram.flashIntensity, 4.0f);
  }
  if (modelProgram.flashEnabled >= 0) {
    glUniform1i(modelProgram.flashEnabled, flashlightOn ? 1 : 0);
  }
  if (modelProgram.shadowEnabled >= 0) {
    glUniform1i(modelProgram.shadowEnabled, flashlightOn ? 1 : 0);
  }
  // debug scene light (L key): lifts ambient so geometry is inspectable
  // without depending on the flashlight cone. flashlight state is untouched.
  // explicit reset on toggle-off is required because drawRenderable never
  // re-sets ambient, so without this the previous frame's value persists
  if (modelProgram.ambientColor >= 0) {
    glUniform3fv(modelProgram.ambientColor, 1,
                 debugLight ? glm::value_ptr(glm::vec3(0.7f, 0.78f, 0.85f))
                            : glm::value_ptr(underwater::kAmbientColor));
  }
  glActiveTexture(GL_TEXTURE4);
  glBindTexture(GL_TEXTURE_2D, shadowMap.depthTexture);

  drawRenderable(modelProgram, sandRenderable);
  // TEMP: skip static + LOD to isolate the bottleneck
  for (const Renderable &renderable : lodScene.staticRenderables) {
    drawRenderable(modelProgram, renderable);
  }

  std::vector<const Renderable *> lodRenderables;
  lodRenderables.reserve(lodScene.groups.size() * 3);
  collectLodRenderables(lodScene, LodTier::High, lodRenderables);
  for (const Renderable *renderable : lodRenderables) {
    drawRenderable(modelProgram, *renderable);
  }
  lodRenderables.clear();
  collectLodRenderables(lodScene, LodTier::Med, lodRenderables);
  for (const Renderable *renderable : lodRenderables) {
    drawRenderable(modelProgram, *renderable);
  }
  lodRenderables.clear();
  collectLodRenderables(lodScene, LodTier::Low, lodRenderables);
  for (const Renderable *renderable : lodRenderables) {
    drawRenderable(modelProgram, *renderable);
  }

  // fish: separate per-instance draw so each fish's model matrix can be
  // updated by the B14 AI every frame (LOD groups have baked VBOs)
  for (const Renderable &renderable : fishRenderables) {
    drawRenderable(modelProgram, renderable);
  }

  // bait: draw the worm mesh at each landed bait. on consume the bait is
  // erased from the vector, so the worm disappears the instant a fish touches it
  constexpr float kBaitMeshScale = 0.18f;
  for (const Bait &b : baits) {
    if (b.consumed) continue;
    Renderable worm = baitMesh;
    glm::mat4 m = glm::translate(glm::mat4(1.0f), b.position);
    // worm body lies on its side on the seabed. amplitude of any motion is
    // proportional to horizontal speed so it only wriggles when actually
    // moving (sinking, drifting), not when settled. frequency is low so it
    // reads as a body wave rather than vibration
    const float horizSpeed =
        glm::length(glm::vec2(b.velocity.x, b.velocity.z));
    if (!b.landed && horizSpeed > 0.05f) {
      const float yaw = std::atan2(b.velocity.z, b.velocity.x);
      m = glm::rotate(m, yaw, glm::vec3(0.0f, 1.0f, 0.0f));
      // amplitude scales with speed (max ~6deg), frequency 5 Hz
      const float amp = std::min(horizSpeed * 0.04f, 0.10f);
      const float roll = std::sin(b.lifetime * 5.0f) * amp;
      m = glm::rotate(m, roll, glm::vec3(1.0f, 0.0f, 0.0f));
      m = glm::rotate(m, glm::half_pi<float>(), glm::vec3(0.0f, 0.0f, 1.0f));
    } else {
      m = glm::rotate(m, b.restYaw, glm::vec3(0.0f, 1.0f, 0.0f));
      m = glm::rotate(m, glm::half_pi<float>(), glm::vec3(0.0f, 0.0f, 1.0f));
    }
    // shrink as it gets eaten so the consumption is visible across the eat
    // window. starts at full scale, reaches zero as eatProgress -> 1
    float scale = kBaitMeshScale * (1.0f - b.eatProgress);
    worm.model = glm::scale(m, glm::vec3(scale));
    drawRenderable(modelProgram, worm);
  }

  // debug patrol loop drawing: toggled with P. samples each loop densely and
  // renders it as a closed line strip so you can see what fish are following
  if (debugPatrols && debugLineProgram != 0) {
    glUseProgram(debugLineProgram);
    glUniformMatrix4fv(debugLineViewLoc, 1, GL_FALSE, glm::value_ptr(view));
    glUniformMatrix4fv(debugLineProjLoc, 1, GL_FALSE, glm::value_ptr(projection));
    glUniformMatrix4fv(debugLineModelLoc, 1, GL_FALSE,
                       glm::value_ptr(glm::mat4(1.0f)));
    // each loop gets a slightly different colour so overlaps are readable
    for (std::size_t li = 0; li < patrols.size(); ++li) {
      const PatrolLoop &loop = patrols[li];
      if (loop.points.size() < 4) continue;
      // dense sample of the Catmull-Rom curve for a smooth visual
      constexpr int kSamples = 96;
      std::vector<glm::vec3> line;
      line.reserve(kSamples + 1);
      for (int s = 0; s <= kSamples; ++s) {
        const float t = static_cast<float>(s) / static_cast<float>(kSamples);
        line.push_back(sampleLoop(loop, t));
      }
      GLuint vao, vbo;
      glGenVertexArrays(1, &vao);
      glGenBuffers(1, &vbo);
      glBindVertexArray(vao);
      glBindBuffer(GL_ARRAY_BUFFER, vbo);
      glBufferData(GL_ARRAY_BUFFER,
                   static_cast<GLsizeiptr>(line.size() * sizeof(glm::vec3)),
                   line.data(), GL_STATIC_DRAW);
      glEnableVertexAttribArray(0);
      glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(glm::vec3),
                            nullptr);
      // colour cycles through warm hues per loop
      const float hue = static_cast<float>(li) * 0.27f;
      const glm::vec3 color(std::sin(hue) * 0.5f + 0.5f,
                            std::sin(hue + 2.0f) * 0.5f + 0.5f,
                            std::sin(hue + 4.0f) * 0.5f + 0.5f);
      glUniform3fv(debugLineColorLoc, 1, glm::value_ptr(color));
      glDrawArrays(GL_LINE_STRIP, 0,
                   static_cast<GLsizei>(line.size()));
      glDeleteBuffers(1, &vbo);
      glDeleteVertexArrays(1, &vao);
    }
  }

  // Skybox drawn last. View matrix is stripped of translation so the cube
  // always surrounds the camera; vertex shader forces depth = 1.0, so we use
  // GL_LEQUAL to let it pass the depth test and never occlude geometry
  if (skyboxRenderable.indexCount > 0) {
    // skybox: view matrix stripped of translation so the cube always surrounds
    // the camera; vertex shader forces depth = w (so depth = 1.0 in NDC) which
    // lets it pass GL_LEQUAL against the cleared depth and never occlude the
    // geometry drawn before it. culling must be off because the camera sits
    // INSIDE the cube, so every visible face is a back-face from outside
    glDepthFunc(GL_LEQUAL);
    glDisable(GL_CULL_FACE);
    glUseProgram(skyboxProgram);
    glm::mat4 skyboxView = glm::mat4(glm::mat3(view));  // drop translation
    if (skyboxViewLoc >= 0) {
      glUniformMatrix4fv(skyboxViewLoc, 1, GL_FALSE,
                         glm::value_ptr(skyboxView));
    }
    if (skyboxProjLoc >= 0) {
      glUniformMatrix4fv(skyboxProjLoc, 1, GL_FALSE,
                         glm::value_ptr(projection));
    }
    if (skyboxFogColorLoc >= 0) {
      glUniform3fv(skyboxFogColorLoc, 1, glm::value_ptr(underwater::kFogColor));
    }
    glActiveTexture(GL_TEXTURE6);
    glBindTexture(GL_TEXTURE_CUBE_MAP, skyboxTexture);
    glBindVertexArray(skyboxRenderable.VAO);
    glDrawElements(GL_TRIANGLES, skyboxRenderable.indexCount, GL_UNSIGNED_INT,
                   nullptr);
    glBindVertexArray(0);
    glEnable(GL_CULL_FACE);
    glDepthFunc(GL_LESS);
  }

  // Goggle overlay: drawn last as a screen-space HUD pass. No depth, blended
  drawGoggles(goggles, sceneTime);

  glfwSwapBuffers(window);
}

void processInput(GLFWwindow *window) {
  if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS) {
    glfwSetWindowShouldClose(window, true);
  }

  // flashlight toggle on F (edge-detected)
  static bool fHeld = false;
  const bool fNow = glfwGetKey(window, GLFW_KEY_F) == GLFW_PRESS;
  if (fNow && !fHeld) flashlight::enabled = !flashlight::enabled;
  fHeld = fNow;

  // debug full-scene light toggle on L (edge-detected): overrides flashlight so
  // you can inspect geometry/material without the spotlight. useful for
  // verifying bait meshes, fish states, prop placement in even lighting
  static bool lHeld = false;
  const bool lNow = glfwGetKey(window, GLFW_KEY_L) == GLFW_PRESS;
  if (lNow && !lHeld) debugLight = !debugLight;
  lHeld = lNow;

  // debug patrol loop overlay on P (edge-detected): draws each Catmull-Rom
  // patrol loop as a coloured line strip so the spline paths are visible
  static bool pHeld = false;
  const bool pNow = glfwGetKey(window, GLFW_KEY_P) == GLFW_PRESS;
  if (pNow && !pHeld) debugPatrols = !debugPatrols;
  pHeld = pNow;

  // scare-all on E (edge-detected): radial shockwave centered on the camera,
  // wider than the flashlight cone
  static bool eHeld = false;
  const bool eNow = glfwGetKey(window, GLFW_KEY_E) == GLFW_PRESS;
  if (eNow && !eHeld) scareAllFish(fish, cameraPosition);
  eHeld = eNow;

  // bait throw on LMB (edge-detected)
  static bool lmbHeld = false;
  const bool lmbNow = glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS;
  if (lmbNow && !lmbHeld) {
    const glm::vec3 fwd = cameraOrientation * glm::vec3(0.0f, 0.0f, -1.0f);
    throwBait(cameraPosition, glm::normalize(fwd));
  }
  lmbHeld = lmbNow;

  glm::vec3 inputDirection(0.0f);
  glm::vec3 independentInputDirection(0.0f);
  if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) inputDirection += glm::vec3(0.0f, 0.0f, -1.0f);
  if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) inputDirection += glm::vec3(0.0f, 0.0f, 1.0f);
  if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) inputDirection += glm::vec3(-1.0f, 0.0f, 0.0f);
  if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) inputDirection += glm::vec3(1.0f, 0.0f, 0.0f);
  if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS) independentInputDirection += glm::vec3(0.0f, 1.0f, 0.0f);
  if (glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS) independentInputDirection += glm::vec3(0.0f, -1.0f, 0.0f);
  updateCameraMovement(inputDirection, independentInputDirection, cameraSpeed, deltaTime);
  clampCameraToScene();
}

void processMouse(GLFWwindow *, double xPos, double yPos) {
  if (firstMouse) {
    lastX = static_cast<float>(xPos);
    lastY = static_cast<float>(yPos);
    firstMouse = false;
    return;
  }
  float deltaX = static_cast<float>(xPos) - lastX;
  float deltaY = static_cast<float>(yPos) - lastY;
  lastX = static_cast<float>(xPos);
  lastY = static_cast<float>(yPos);

    // ignore one-off spikes from the cursor-grab
  if (std::abs(deltaX) > 200.0f || std::abs(deltaY) > 200.0f) {
    return;
  }

  rotateCamera(-deltaX, -deltaY, mouseSensitivity);
}

void renderLoop(GLFWwindow *window) {
  lastTime = static_cast<float>(glfwGetTime());
  // fps shown in the window title, averaged over a short window so the number
  // is readable instead of flickering every frame
  float fpsAccum = 0.0f;
  int fpsFrames = 0;
  while (!glfwWindowShouldClose(window)) {
    glfwPollEvents();
    float time = static_cast<float>(glfwGetTime());
    deltaTime = time - lastTime;
    lastTime = time;

    fpsAccum += deltaTime;
    ++fpsFrames;
    if (fpsAccum >= 0.5f) {
      const int fps = static_cast<int>(fpsFrames / fpsAccum + 0.5f);
      char title[64];
      std::snprintf(title, sizeof(title), "Underwater Scene | %d FPS", fps);
      glfwSetWindowTitle(window, title);
      fpsAccum = 0.0f;
      fpsFrames = 0;
    }

    processInput(window);
    updateFish(fish, deltaTime, time, patrols, flashlight::enabled,
               flashlight::position());
    // bait update runs after fish so the feeder count sees this frame's
    // positions; worms shrink faster the more fish crowd them
    updateBait(baits, deltaTime, fish);
    renderScene(window);
  }
}

void shutdown(GLFWwindow *) {
  destroyLodScene(lodScene);
  std::vector<Renderable> sandOnly{sandRenderable};
  destroyRenderables(sandOnly);
  destroyShadowMap(shadowMap);
  destroyGoggles(goggles);
  glDeleteTextures(1, &skyboxTexture);
  glDeleteVertexArrays(1, &skyboxRenderable.VAO);
  glDeleteBuffers(1, &skyboxRenderable.VBO);
  glDeleteBuffers(1, &skyboxRenderable.EBO);
  deleteProgram(skyboxProgram);
  deleteProgram(shadowProgram.id);
  deleteProgram(modelProgram.id);
}
