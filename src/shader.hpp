#pragma once

#include <GL/glew.h>

#include <fstream>
#include <iostream>
#include <sstream>
#include <string>

inline std::string shaderReadFile(const char *path) {
  std::ifstream file(path);
  if (!file) {
    std::cerr << "Failed to open " << path << std::endl;
    return {};
  }

  std::stringstream buffer;
  buffer << file.rdbuf();
  return buffer.str();
}

inline GLuint shaderCreateFromFile(GLenum type, const char *path) {
  std::string source = shaderReadFile(path);
  const char *sourcePtr = source.c_str();

  GLuint shader = glCreateShader(type);
  glShaderSource(shader, 1, &sourcePtr, nullptr);
  glCompileShader(shader);

  GLint ok = GL_FALSE;
  glGetShaderiv(shader, GL_COMPILE_STATUS, &ok);
  if (!ok) {
    char log[1024];
    glGetShaderInfoLog(shader, sizeof(log), nullptr, log);
    std::cerr << "Shader compile error: " << log << std::endl;
  }

  return shader;
}

inline GLuint createProgram(const char *vertexPath, const char *fragmentPath) {
  GLuint vertexShader = shaderCreateFromFile(GL_VERTEX_SHADER, vertexPath);
  GLuint fragmentShader = shaderCreateFromFile(GL_FRAGMENT_SHADER, fragmentPath);
  GLuint newProgram = glCreateProgram();

  glAttachShader(newProgram, vertexShader);
  glAttachShader(newProgram, fragmentShader);
  glLinkProgram(newProgram);

  GLint ok = GL_FALSE;
  glGetProgramiv(newProgram, GL_LINK_STATUS, &ok);
  if (!ok) {
    char log[1024];
    glGetProgramInfoLog(newProgram, sizeof(log), nullptr, log);
    std::cerr << "Program link error: " << log << std::endl;
  }

  glDeleteShader(vertexShader);
  glDeleteShader(fragmentShader);
  return newProgram;
}

inline void deleteProgram(GLuint program) { glDeleteProgram(program); }
