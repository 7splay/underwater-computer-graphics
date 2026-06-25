#pragma once

#include "billboard.hpp"
#include "instancing.hpp"

#include <glm/glm.hpp>

#include <vector>

enum class LodTier { High = 0, Med = 1, Low = 2 };

struct LodInstancedGroup {
  Renderable high{};
  Renderable med{};
  Renderable low{};
  std::vector<Renderable> extraHigh;
  std::vector<Renderable> extraMed;
  std::vector<Renderable> extraLow;
  std::vector<glm::mat4> sourceMatrices;
  std::vector<glm::vec3> worldPositions;
  float highDistance = 18.0f;
  float medDistance = 58.0f;
  float billboardWidth = 1.4f;
  float billboardHeight = 1.8f;
  bool castsShadow = true;
  bool lowUsesBillboard = true;
};

struct LodScene {
  std::vector<LodInstancedGroup> groups;
  std::vector<Renderable> staticRenderables;
};

inline glm::vec3 extractWorldPosition(const glm::mat4 &transform) {
  return glm::vec3(transform[3]);
}

inline void initLodInstancedGroupTransforms(LodInstancedGroup &group,
                                            const std::vector<glm::mat4> &transforms,
                                            float highDistance, float medDistance,
                                            float billboardWidth,
                                            float billboardHeight,
                                            bool castsShadow) {
  group.highDistance = highDistance;
  group.medDistance = medDistance;
  group.billboardWidth = billboardWidth;
  group.billboardHeight = billboardHeight;
  group.castsShadow = castsShadow;
  group.sourceMatrices = transforms;
  group.worldPositions.clear();
  group.worldPositions.reserve(transforms.size());
  for (const glm::mat4 &transform : transforms) {
    group.worldPositions.push_back(extractWorldPosition(transform));
  }
}

inline bool isCrossPlantMesh(const Renderable &mesh) {
  return mesh.indexCount > 0 && mesh.indexCount <= 12;
}

inline Renderable createLodMedRenderable(const Renderable &baseMesh,
                                         int medDecimation) {
  if (isCrossPlantMesh(baseMesh)) {
    Renderable med = cloneMeshWithOwnVao(baseMesh);
    med.useNormalMap = false;
    med.normalStrength = 0.0f;
    return med;
  }
  return createSimplifiedMesh(baseMesh, medDecimation);
}

inline Renderable createLodLowRenderable(const Renderable &baseMesh,
                                         bool lowUsesBillboard,
                                         float billboardWidth,
                                         float billboardHeight,
                                         int lowDecimation) {
  if (lowUsesBillboard) {
    return createBillboardRenderable(baseMesh, billboardWidth, billboardHeight);
  }
  return createSimplifiedMesh(baseMesh, lowDecimation);
}

inline LodInstancedGroup
createLodInstancedGroup(const Renderable &baseMesh,
                        const std::vector<glm::mat4> &transforms,
                        float highDistance, float medDistance,
                        float billboardWidth, float billboardHeight,
                        bool castsShadow = true, bool lowUsesBillboard = true,
                        int medDecimation = 3, int lowDecimation = 10) {
  LodInstancedGroup group{};
  initLodInstancedGroupTransforms(group, transforms, highDistance, medDistance,
                                  billboardWidth, billboardHeight, castsShadow);
  group.lowUsesBillboard = lowUsesBillboard;

  group.high = cloneMeshWithOwnVao(baseMesh);
  group.med = createLodMedRenderable(baseMesh, medDecimation);
  group.low = createLodLowRenderable(baseMesh, lowUsesBillboard, billboardWidth,
                                     billboardHeight, lowDecimation);
  return group;
}

inline LodInstancedGroup createLodInstancedMultiMeshGroup(
    const std::vector<Renderable> &baseMeshes,
    const std::vector<glm::mat4> &transforms, float highDistance,
    float medDistance, float billboardWidth, float billboardHeight,
    bool castsShadow = true, bool lowUsesBillboard = true,
    int medDecimation = 3, int lowDecimation = 10) {
  if (baseMeshes.empty()) {
    return {};
  }

  LodInstancedGroup group = createLodInstancedGroup(
      baseMeshes.front(), transforms, highDistance, medDistance, billboardWidth,
      billboardHeight, castsShadow, lowUsesBillboard, medDecimation,
      lowDecimation);

  for (std::size_t meshIndex = 1; meshIndex < baseMeshes.size(); ++meshIndex) {
    const Renderable &baseMesh = baseMeshes[meshIndex];
    group.extraHigh.push_back(cloneMeshWithOwnVao(baseMesh));
    group.extraMed.push_back(createLodMedRenderable(baseMesh, medDecimation));
    group.extraLow.push_back(createLodLowRenderable(
        baseMesh, lowUsesBillboard, billboardWidth, billboardHeight,
        lowDecimation));
  }

  return group;
}

inline void uploadLodTierMatrices(LodInstancedGroup &group, Renderable &renderable,
                                  const std::vector<glm::mat4> &matrices) {
  uploadInstanceMatrices(renderable, matrices);
}

inline void updateLodInstancedGroup(LodInstancedGroup &group,
                                    const glm::vec3 &cameraPos) {
  std::vector<glm::mat4> highMatrices;
  std::vector<glm::mat4> medMatrices;
  std::vector<glm::mat4> lowMatrices;
  highMatrices.reserve(group.sourceMatrices.size());
  medMatrices.reserve(group.sourceMatrices.size() / 2);
  lowMatrices.reserve(group.sourceMatrices.size() / 3);

  for (std::size_t i = 0; i < group.sourceMatrices.size(); ++i) {
    const float distance =
        glm::length(cameraPos - group.worldPositions[i]);
    if (distance < group.highDistance) {
      highMatrices.push_back(group.sourceMatrices[i]);
    } else if (distance < group.medDistance) {
      medMatrices.push_back(group.sourceMatrices[i]);
    } else if (group.lowUsesBillboard) {
      const float scale = extractUniformScale(group.sourceMatrices[i]);
      lowMatrices.push_back(billboardInstanceMatrix(
          group.sourceMatrices[i], group.billboardWidth * scale,
          group.billboardHeight * scale));
    } else {
      lowMatrices.push_back(group.sourceMatrices[i]);
    }
  }

  uploadLodTierMatrices(group, group.high, highMatrices);
  uploadLodTierMatrices(group, group.med, medMatrices);
  uploadLodTierMatrices(group, group.low, lowMatrices);

  for (std::size_t meshIndex = 0; meshIndex < group.extraHigh.size(); ++meshIndex) {
    uploadLodTierMatrices(group, group.extraHigh[meshIndex], highMatrices);
    uploadLodTierMatrices(group, group.extraMed[meshIndex], medMatrices);
    uploadLodTierMatrices(group, group.extraLow[meshIndex], lowMatrices);
  }
}

inline void updateLodScene(LodScene &scene, const glm::vec3 &cameraPos) {
  for (LodInstancedGroup &group : scene.groups) {
    updateLodInstancedGroup(group, cameraPos);
  }
}

inline void collectLodRenderableIfVisible(const Renderable &renderable,
                                          std::vector<const Renderable *> &out) {
  if (renderable.instanceCount > 0 && renderable.instVBO != 0) {
    out.push_back(&renderable);
  }
}

inline void collectLodRenderables(const LodScene &scene, LodTier tier,
                                  std::vector<const Renderable *> &out) {
  for (const LodInstancedGroup &group : scene.groups) {
    switch (tier) {
    case LodTier::High:
      collectLodRenderableIfVisible(group.high, out);
      for (const Renderable &renderable : group.extraHigh) {
        collectLodRenderableIfVisible(renderable, out);
      }
      break;
    case LodTier::Med:
      collectLodRenderableIfVisible(group.med, out);
      for (const Renderable &renderable : group.extraMed) {
        collectLodRenderableIfVisible(renderable, out);
      }
      break;
    case LodTier::Low:
      collectLodRenderableIfVisible(group.low, out);
      for (const Renderable &renderable : group.extraLow) {
        collectLodRenderableIfVisible(renderable, out);
      }
      break;
    }
  }
}

inline void destroyLodScene(LodScene &scene) {
  std::vector<Renderable> disposable;
  for (LodInstancedGroup &group : scene.groups) {
    disposable.push_back(group.high);
    disposable.push_back(group.med);
    disposable.push_back(group.low);
    disposable.insert(disposable.end(), group.extraHigh.begin(),
                      group.extraHigh.end());
    disposable.insert(disposable.end(), group.extraMed.begin(),
                      group.extraMed.end());
    disposable.insert(disposable.end(), group.extraLow.begin(),
                      group.extraLow.end());
  }
  disposable.insert(disposable.end(), scene.staticRenderables.begin(),
                    scene.staticRenderables.end());
  destroyRenderables(disposable);
  scene.groups.clear();
  scene.staticRenderables.clear();
}
