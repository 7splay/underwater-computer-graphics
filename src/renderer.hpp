#pragma once

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
  GLint normalStrength = -1;
  GLint metallic = -1;
  GLint roughness = -1;
  GLint albedoTint = -1;
  GLint fogColor = -1;
  GLint fogDensity = -1;
  GLint fogMax = -1;
  GLint ambientColor = -1;
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
  program.normalStrength = uniformLocation(program.id, "uNormalStrength");
  program.metallic = uniformLocation(program.id, "uMetallic");
  program.roughness = uniformLocation(program.id, "uRoughness");
  program.albedoTint = uniformLocation(program.id, "uAlbedoTint");
  program.fogColor = uniformLocation(program.id, "uFogColor");
  program.fogDensity = uniformLocation(program.id, "uFogDensity");
  program.fogMax = uniformLocation(program.id, "uFogMax");
  program.ambientColor = uniformLocation(program.id, "uAmbientColor");

  const GLint texture = uniformLocation(program.id, "uTexture");
  const GLint normalMap = uniformLocation(program.id, "uNormalMap");
  const GLint roughnessMap = uniformLocation(program.id, "uRoughnessMap");
  const GLint metallicMap = uniformLocation(program.id, "uMetallicMap");
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

inline void bindRenderableTextures(const Renderable &renderable) {
  glActiveTexture(GL_TEXTURE0);
  glBindTexture(GL_TEXTURE_2D, renderable.texture);

  glActiveTexture(GL_TEXTURE1);
  glBindTexture(GL_TEXTURE_2D, renderable.normalTexture != 0
                                  ? renderable.normalTexture
                                  : renderable.texture);

  glActiveTexture(GL_TEXTURE2);
  glBindTexture(GL_TEXTURE_2D, renderable.roughnessTexture != 0
                                  ? renderable.roughnessTexture
                                  : renderable.texture);

  glActiveTexture(GL_TEXTURE3);
  glBindTexture(GL_TEXTURE_2D, renderable.metallicTexture != 0
                                  ? renderable.metallicTexture
                                  : renderable.texture);
}

inline void drawRenderable(const ModelProgram &program,
                           const Renderable &renderable) {
  if (renderable.indexCount <= 0) {
    return;
  }

  const bool useInstancing =
      renderable.instanceCount > 1 && renderable.instVBO != 0;

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

  glBindVertexArray(renderable.VAO);
  glEnable(GL_POLYGON_OFFSET_FILL);
  glPolygonOffset(1.0f, 1.0f);
  if (useInstancing) {
    glDrawElementsInstanced(GL_TRIANGLES, renderable.indexCount,
                            GL_UNSIGNED_INT, nullptr, renderable.instanceCount);
  } else {
    glDrawElements(GL_TRIANGLES, renderable.indexCount, GL_UNSIGNED_INT,
                   nullptr);
  }
  glDisable(GL_POLYGON_OFFSET_FILL);
}
