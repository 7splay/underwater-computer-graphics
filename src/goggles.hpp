#pragma once

#include <GL/glew.h>

#include "shader.hpp"

// fullscreen overlay for the goggle vignette, drawn last with no depth
//   sigma:    radius of each eye window
//   eyeSep:   distance between the two eye centers
//   darkness: frame opacity
struct Goggles {
  GLuint vao = 0;
  GLuint vbo = 0;
  GLuint program = 0;
  GLint resolutionLoc = -1;
  GLint timeLoc = -1;
  GLint sigmaLoc = -1;
  GLint eyeSepLoc = -1;
  GLint darknessLoc = -1;
  float sigma = 0.35f;
  float eyeSep = 0.40f;
  float darkness = 1.0f;
};

inline Goggles createGoggles() {
  Goggles g{};

  // one oversized triangle covering the whole screen
  const float verts[] = {
      -1.0f, -1.0f, 3.0f, -1.0f, -1.0f, 3.0f,
  };

  glGenVertexArrays(1, &g.vao);
  glGenBuffers(1, &g.vbo);
  glBindVertexArray(g.vao);
  glBindBuffer(GL_ARRAY_BUFFER, g.vbo);
  glBufferData(GL_ARRAY_BUFFER, sizeof(verts), verts, GL_STATIC_DRAW);
  glEnableVertexAttribArray(0);
  glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), nullptr);
  glBindVertexArray(0);

  g.program = createProgram("shaders/goggles.vert", "shaders/goggles.frag");
  glUseProgram(g.program);
  g.resolutionLoc = glGetUniformLocation(g.program, "uResolution");
  g.timeLoc = glGetUniformLocation(g.program, "uTime");
  g.sigmaLoc = glGetUniformLocation(g.program, "uSigma");
  g.eyeSepLoc = glGetUniformLocation(g.program, "uEyeSep");
  g.darknessLoc = glGetUniformLocation(g.program, "uDarkness");

  return g;
}

inline void drawGoggles(Goggles &g, float currentTime) {
  GLint viewport[4];
  glGetIntegerv(GL_VIEWPORT, viewport);
  const float w = static_cast<float>(viewport[2]);
  const float h = static_cast<float>(viewport[3]);

  glUseProgram(g.program);
  if (g.resolutionLoc >= 0) {
    glUniform2f(g.resolutionLoc, w, h);
  }
  if (g.timeLoc >= 0) {
    glUniform1f(g.timeLoc, currentTime);
  }
  if (g.sigmaLoc >= 0) {
    glUniform1f(g.sigmaLoc, g.sigma);
  }
  if (g.eyeSepLoc >= 0) {
    glUniform1f(g.eyeSepLoc, g.eyeSep);
  }
  if (g.darknessLoc >= 0) {
    glUniform1f(g.darknessLoc, g.darkness);
  }

  // sits on top of everything, never writes depth
  glDisable(GL_DEPTH_TEST);
  glDepthMask(GL_FALSE);
  glEnable(GL_BLEND);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

  glBindVertexArray(g.vao);
  glDrawArrays(GL_TRIANGLES, 0, 3);
  glBindVertexArray(0);

  glDisable(GL_BLEND);
  glDepthMask(GL_TRUE);
  glEnable(GL_DEPTH_TEST);
}

inline void destroyGoggles(Goggles &g) {
  if (g.vao != 0) {
    glDeleteVertexArrays(1, &g.vao);
    g.vao = 0;
  }
  if (g.vbo != 0) {
    glDeleteBuffers(1, &g.vbo);
    g.vbo = 0;
  }
  if (g.program != 0) {
    glDeleteProgram(g.program);
    g.program = 0;
  }
}
