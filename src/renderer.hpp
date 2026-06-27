#pragma once

#include "instancing.hpp"
#include "renderable.hpp"
#include "shader.hpp"
#include "underwater_atmosphere.hpp"

#include <GL/glew.h>

#include <glm/gtc/type_ptr.hpp>

struct ModelProgram {
  GLuint id = 0;
  GLint view = -1;
  GLint projection = -1;
  GLint model = -1;
  GLint cameraPos = -1;
  GLint useInstancing = -1;
  GLint useNormalMap = -1;
  GLint useRoughnessMap = -1;
  GLint useMetallicMap = -1;
  GLint useArmMap = -1;
  GLint useAlphaTest = -1;
  GLint alphaCutoff = -1;
  GLint normalStrength = -1;
  GLint metallic = -1;
  GLint roughness = -1;
  GLint albedoTint = -1;
  GLint fogColor = -1;
  GLint fogDensity = -1;
  GLint fogMax = -1;
  GLint ambientColor = -1;
  GLint lightSpace = -1;
  GLint flashPos = -1;
  GLint flashDir = -1;
  GLint flashRange = -1;
  GLint flashCosInner = -1;
  GLint flashCosOuter = -1;
  GLint flashColor = -1;
  GLint flashIntensity = -1;
  GLint flashEnabled = -1;
  GLint shadowEnabled = -1;
  GLint useBillboard = -1;
  GLint useAlphaCutout = -1;
  GLint useOpacityCutout = -1;
};

inline GLint uniformLocation(GLuint program, const char *name) {
  return glGetUniformLocation(program, name);
}

inline ModelProgram createModelProgram() {
  ModelProgram program{};
  program.id = createProgram("shaders/model.vert", "shaders/model.frag");

  glUseProgram(program.id);
  program.view = uniformLocation(program.id, "uView");
  program.projection = uniformLocation(program.id, "uProjection");
  program.model = uniformLocation(program.id, "uModel");
  program.cameraPos = uniformLocation(program.id, "uCameraPos");
  program.useInstancing = uniformLocation(program.id, "uUseInstancing");
  program.useNormalMap = uniformLocation(program.id, "uUseNormalMap");
  program.useRoughnessMap = uniformLocation(program.id, "uUseRoughnessMap");
  program.useMetallicMap = uniformLocation(program.id, "uUseMetallicMap");
  program.useArmMap = uniformLocation(program.id, "uUseArmMap");
  program.useAlphaTest = uniformLocation(program.id, "uUseAlphaTest");
  program.alphaCutoff = uniformLocation(program.id, "uAlphaCutoff");
  program.normalStrength = uniformLocation(program.id, "uNormalStrength");
  program.metallic = uniformLocation(program.id, "uMetallic");
  program.roughness = uniformLocation(program.id, "uRoughness");
  program.albedoTint = uniformLocation(program.id, "uAlbedoTint");
  program.fogColor = uniformLocation(program.id, "uFogColor");
  program.fogDensity = uniformLocation(program.id, "uFogDensity");
  program.fogMax = uniformLocation(program.id, "uFogMax");
  program.ambientColor = uniformLocation(program.id, "uAmbientColor");
  program.lightSpace = uniformLocation(program.id, "uLightSpace");
  program.flashPos = uniformLocation(program.id, "uFlashPos");
  program.flashDir = uniformLocation(program.id, "uFlashDir");
  program.flashRange = uniformLocation(program.id, "uFlashRange");
  program.flashCosInner = uniformLocation(program.id, "uFlashCosInner");
  program.flashCosOuter = uniformLocation(program.id, "uFlashCosOuter");
  program.flashColor = uniformLocation(program.id, "uFlashColor");
  program.flashIntensity = uniformLocation(program.id, "uFlashIntensity");
  program.flashEnabled = uniformLocation(program.id, "uFlashEnabled");
  program.shadowEnabled = uniformLocation(program.id, "uShadowEnabled");
  program.useBillboard = uniformLocation(program.id, "uBillboard");
  program.useAlphaCutout = uniformLocation(program.id, "uUseAlphaCutout");
  program.useOpacityCutout = uniformLocation(program.id, "uUseOpacityCutout");

  const GLint texture = uniformLocation(program.id, "uTexture");
  const GLint normalMap = uniformLocation(program.id, "uNormalMap");
  const GLint roughnessMap = uniformLocation(program.id, "uRoughnessMap");
  const GLint metallicMap = uniformLocation(program.id, "uMetallicMap");
  const GLint opacityMap = uniformLocation(program.id, "uOpacityMap");
  const GLint shadowMap = uniformLocation(program.id, "uShadowMap");
  if (texture >= 0) {
    glUniform1i(texture, 0);
  }
  if (normalMap >= 0) {
    glUniform1i(normalMap, 1);
  }
  if (roughnessMap >= 0) {
    glUniform1i(roughnessMap, 2);
  }
  if (metallicMap >= 0) {
    glUniform1i(metallicMap, 3);
  }
  if (opacityMap >= 0) {
    glUniform1i(opacityMap, 5);
  }
  if (shadowMap >= 0) {
    glUniform1i(shadowMap, 4);  // texture unit 4 reserved for the shadow map
  }

  if (program.fogColor >= 0) {
    glUniform3fv(program.fogColor, 1, glm::value_ptr(underwater::kFogColor));
  }
  if (program.fogDensity >= 0) {
    glUniform1f(program.fogDensity, underwater::kFogDensity);
  }
  if (program.fogMax >= 0) {
    glUniform1f(program.fogMax, underwater::kFogMax);
  }
  if (program.ambientColor >= 0) {
    glUniform3fv(program.ambientColor, 1,
                 glm::value_ptr(underwater::kAmbientColor));
  }

  return program;
}

// 1x1 white texture used whenever a renderable has no diffuse map, so every
// sampler unit always points at a complete texture. without this, binding 0
// makes macOS log "unit 0 ... unloadable ... using zero texture"
inline GLuint whiteFallbackTexture() {
  static GLuint tex = 0;
  if (tex == 0) {
    const unsigned char white[3] = {255u, 255u, 255u};
    glGenTextures(1, &tex);
    glBindTexture(GL_TEXTURE_2D, tex);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, 1, 1, 0, GL_RGB, GL_UNSIGNED_BYTE,
                 white);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  }
  return tex;
}

inline void bindRenderableTextures(const Renderable &renderable) {
  const GLuint base =
      renderable.texture != 0 ? renderable.texture : whiteFallbackTexture();
  glActiveTexture(GL_TEXTURE0);
  glBindTexture(GL_TEXTURE_2D, base);

  glActiveTexture(GL_TEXTURE1);
  glBindTexture(GL_TEXTURE_2D,
                renderable.normalTexture != 0 ? renderable.normalTexture : base);

  glActiveTexture(GL_TEXTURE2);
  glBindTexture(GL_TEXTURE_2D, renderable.roughnessTexture != 0
                                  ? renderable.roughnessTexture
                                  : base);

  glActiveTexture(GL_TEXTURE3);
  glBindTexture(GL_TEXTURE_2D, renderable.metallicTexture != 0
                                  ? renderable.metallicTexture
                                  : base);

  glActiveTexture(GL_TEXTURE5);
  glBindTexture(GL_TEXTURE_2D, renderable.opacityTexture != 0
                                  ? renderable.opacityTexture
                                  : base);
}

inline void drawRenderable(const ModelProgram &program,
                           const Renderable &renderable) {
  if (renderable.indexCount <= 0) {
    return;
  }
  if (renderable.instVBO != 0 && renderable.instanceCount <= 0) {
    return;
  }

  const bool useInstancing =
      renderable.instVBO != 0 && renderable.instanceCount > 0;

  if (program.useInstancing >= 0) {
    glUniform1i(program.useInstancing, useInstancing ? 1 : 0);
  }
  if (!useInstancing && program.model >= 0) {
    glUniformMatrix4fv(program.model, 1, GL_FALSE,
                       glm::value_ptr(renderable.model));
  }

  bindRenderableTextures(renderable);

  if (program.useNormalMap >= 0) {
    glUniform1i(program.useNormalMap, renderable.useNormalMap ? 1 : 0);
  }
  if (program.useRoughnessMap >= 0) {
    glUniform1i(program.useRoughnessMap, renderable.useRoughnessMap ? 1 : 0);
  }
  if (program.useMetallicMap >= 0) {
    glUniform1i(program.useMetallicMap, renderable.useMetallicMap ? 1 : 0);
  }
  if (program.useArmMap >= 0) {
    glUniform1i(program.useArmMap, renderable.useArmMap ? 1 : 0);
  }
  if (program.useAlphaTest >= 0) {
    glUniform1i(program.useAlphaTest, renderable.useAlphaTest ? 1 : 0);
  }
  if (program.alphaCutoff >= 0) {
    glUniform1f(program.alphaCutoff, renderable.alphaCutoff);
  }
  if (program.normalStrength >= 0) {
    glUniform1f(program.normalStrength, renderable.normalStrength);
  }
  if (program.metallic >= 0) {
    glUniform1f(program.metallic, renderable.metallic);
  }
  if (program.roughness >= 0) {
    glUniform1f(program.roughness, renderable.roughness);
  }
  if (program.albedoTint >= 0) {
    glUniform3fv(program.albedoTint, 1, glm::value_ptr(renderable.albedoTint));
  }
  if (program.useBillboard >= 0) {
    glUniform1i(program.useBillboard, renderable.isBillboard ? 1 : 0);
  }
  if (program.useAlphaCutout >= 0) {
    glUniform1i(program.useAlphaCutout, renderable.useAlphaCutout ? 1 : 0);
  }
  if (program.useOpacityCutout >= 0) {
    glUniform1i(program.useOpacityCutout,
                  renderable.useOpacityCutout ? 1 : 0);
  }

  const bool disableCull = renderable.isDoubleSided;
  if (disableCull) {
    glDisable(GL_CULL_FACE);
    if (renderable.useAlphaCutout) {
      glEnable(GL_POLYGON_OFFSET_FILL);
      glPolygonOffset(-1.0f, -2.0f);
    }
  }

  glBindVertexArray(renderable.VAO);
  if (useInstancing) {
    bindInstancedAttributes(renderable.instVBO);
  }
  if (useInstancing) {
    glDrawElementsInstanced(GL_TRIANGLES, renderable.indexCount,
                            GL_UNSIGNED_INT, nullptr, renderable.instanceCount);
  } else {
    glDrawElements(GL_TRIANGLES, renderable.indexCount, GL_UNSIGNED_INT,
                   nullptr);
  }
  if (disableCull) {
    if (renderable.useAlphaCutout) {
      glDisable(GL_POLYGON_OFFSET_FILL);
    }
    glEnable(GL_CULL_FACE);
  }
}

// shadow-pass draw: reuses the lit-pass geometry with a depth-only program,
// only needs uLightSpace, uModel and uUseInstancing
struct ShadowProgram {
  GLuint id = 0;
  GLint lightSpace = -1;
  GLint model = -1;
  GLint useInstancing = -1;
  GLint alphaMode = -1;
  GLint alphaCutoff = -1;
};

inline ShadowProgram createShadowProgram() {
  ShadowProgram sp{};
  sp.id = createProgram("shaders/shadow.vert", "shaders/shadow.frag");
  glUseProgram(sp.id);
  sp.lightSpace = uniformLocation(sp.id, "uLightSpace");
  sp.model = uniformLocation(sp.id, "uModel");
  sp.useInstancing = uniformLocation(sp.id, "uUseInstancing");
  sp.alphaMode = uniformLocation(sp.id, "uAlphaMode");
  sp.alphaCutoff = uniformLocation(sp.id, "uAlphaCutoff");
  const GLint tex = uniformLocation(sp.id, "uTexture");
  if (tex >= 0) glUniform1i(tex, 0);
  const GLint opacity = uniformLocation(sp.id, "uOpacityMap");
  if (opacity >= 0) glUniform1i(opacity, 1);
  return sp;
}

inline void drawRenderableShadow(const ShadowProgram &sp,
                                 const Renderable &renderable,
                                 const glm::mat4 &lightSpace) {
  // billboards are camera-facing impostors; the depth pass has no camera so
  // they'd cast a flat misoriented quad. everything else (incl. cutout meshes)
  // is rendered, with alpha-testing below to keep seaweed/fin silhouettes
  if (renderable.indexCount <= 0 || renderable.isBillboard) {
    return;
  }
  if (renderable.instVBO != 0 && renderable.instanceCount <= 0) {
    return;
  }
  glUseProgram(sp.id);
  if (sp.lightSpace >= 0) {
    glUniformMatrix4fv(sp.lightSpace, 1, GL_FALSE,
                       glm::value_ptr(lightSpace));
  }

  // pick the alpha-test mode matching the lit pass so casters discard the same
  // transparent texels. solid meshes (mode 0) skip the texture sample entirely
  int alphaMode = 0;
  if (renderable.useAlphaCutout) {
    alphaMode = renderable.useOpacityCutout ? 2 : 3;
  } else if (renderable.useAlphaTest) {
    alphaMode = 1;
  }
  if (sp.alphaMode >= 0) {
    glUniform1i(sp.alphaMode, alphaMode);
  }
  if (sp.alphaCutoff >= 0) {
    glUniform1f(sp.alphaCutoff, renderable.alphaCutoff);
  }
  if (alphaMode == 2) {
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, renderable.opacityTexture);
  } else if (alphaMode != 0) {
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, renderable.texture);
  }

  const bool useInstancing =
      renderable.instVBO != 0 && renderable.instanceCount > 0;
  if (sp.useInstancing >= 0) {
    glUniform1i(sp.useInstancing, useInstancing ? 1 : 0);
  }
  if (!useInstancing && sp.model >= 0) {
    glUniformMatrix4fv(sp.model, 1, GL_FALSE,
                       glm::value_ptr(renderable.model));
  }

  glBindVertexArray(renderable.VAO);
  if (useInstancing) {
    bindInstancedAttributes(renderable.instVBO);
    glDrawElementsInstanced(GL_TRIANGLES, renderable.indexCount,
                            GL_UNSIGNED_INT, nullptr, renderable.instanceCount);
  } else {
    glDrawElements(GL_TRIANGLES, renderable.indexCount, GL_UNSIGNED_INT,
                   nullptr);
  }
}
