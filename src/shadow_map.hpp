#pragma once

#include <GL/glew.h>

// depth-only framebuffer for the flashlight shadow map
struct ShadowMap {
  GLuint fbo = 0;
  GLuint depthTexture = 0;
  GLsizei width = 0;
  GLsizei height = 0;
};

inline ShadowMap createShadowMap(GLsizei width = 1024, GLsizei height = 1024) {
  ShadowMap sm{};
  sm.width = width;
  sm.height = height;

  glGenTextures(1, &sm.depthTexture);
  glBindTexture(GL_TEXTURE_2D, sm.depthTexture);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, width, height, 0,
               GL_DEPTH_COMPONENT, GL_FLOAT, nullptr);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  // white border so samples outside the map read as unshadowed
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
  const GLfloat borderColor[] = {1.0f, 1.0f, 1.0f, 1.0f};
  glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, borderColor);
  // hardware PCF: compare against depth instead of plain sampling
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_MODE,
                  GL_COMPARE_REF_TO_TEXTURE);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_FUNC, GL_LESS);

  glGenFramebuffers(1, &sm.fbo);
  glBindFramebuffer(GL_FRAMEBUFFER, sm.fbo);
  glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D,
                         sm.depthTexture, 0);
  glDrawBuffer(GL_NONE);
  glReadBuffer(GL_NONE);
  glBindFramebuffer(GL_FRAMEBUFFER, 0);

  return sm;
}

inline void destroyShadowMap(ShadowMap &sm) {
  if (sm.fbo != 0) {
    glDeleteFramebuffers(1, &sm.fbo);
    sm.fbo = 0;
  }
  if (sm.depthTexture != 0) {
    glDeleteTextures(1, &sm.depthTexture);
    sm.depthTexture = 0;
  }
}

// bind shadow FBO and clear depth; caller restores the main viewport after
inline void bindShadowMapForWriting(const ShadowMap &sm) {
  glBindFramebuffer(GL_FRAMEBUFFER, sm.fbo);
  glViewport(0, 0, sm.width, sm.height);
  glClear(GL_DEPTH_BUFFER_BIT);
}

inline void unbindShadowMap(GLint mainViewportX, GLint mainViewportY,
                            GLsizei mainViewportW, GLsizei mainViewportH) {
  glBindFramebuffer(GL_FRAMEBUFFER, 0);
  glViewport(mainViewportX, mainViewportY, mainViewportW, mainViewportH);
}
