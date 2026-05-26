#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <assimp/Importer.hpp>
#include <assimp/postprocess.h>
#include <assimp/scene.h>

#include <fstream>
#include <iostream>
#include <sstream>
#include <string>

GLuint program = 0;
GLuint VAO = 0;
GLuint VBO = 0;

std::string readFile(const char *path) {
  std::ifstream file(path);
  if (!file) {
    std::cerr << "Failed to open " << path << std::endl;
    return {};
  }

  std::stringstream buffer;
  buffer << file.rdbuf();
  return buffer.str();
}

GLuint createShader(GLenum type, const char *path) {
  std::string source = readFile(path);
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

GLuint createProgram(const char *vertexPath, const char *fragmentPath) {
  GLuint vertexShader = createShader(GL_VERTEX_SHADER, vertexPath);
  GLuint fragmentShader = createShader(GL_FRAGMENT_SHADER, fragmentPath);
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

void framebuffer_size_callback(GLFWwindow *, int width, int height) {
  glViewport(0, 0, width, height);
}

void initTriangle() {
  float vertices[] = {
      -0.5f, -0.4f, 0.0f, 1.0f, 0.2f, 0.2f,
       0.5f, -0.4f, 0.0f, 0.2f, 1.0f, 0.2f,
       0.0f,  0.4f, 0.0f, 0.2f, 0.4f, 1.0f,
  };

  glGenVertexArrays(1, &VAO);
  glGenBuffers(1, &VBO);

  glBindVertexArray(VAO);
  glBindBuffer(GL_ARRAY_BUFFER, VBO);
  glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

  glProvokingVertex(GL_FIRST_VERTEX_CONVENTION);

  glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), nullptr);
  glEnableVertexAttribArray(0);

  glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float),
                        reinterpret_cast<void *>(3 * sizeof(float)));
  glEnableVertexAttribArray(1);
}
void init(GLFWwindow *window) {
  glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
  glViewport(0, 0, 800, 600);

  program = createProgram("shaders/triangle.vert", "shaders/triangle.frag");
  initTriangle();
  // loadModelInfo("models/fish.obj");
}

void renderScene(GLFWwindow *window) {
  glClearColor(0.05f, 0.15f, 0.25f, 1.0f);
  glClear(GL_COLOR_BUFFER_BIT);

  glUseProgram(program);
  glBindVertexArray(VAO);
  glDrawArrays(GL_TRIANGLES, 0, 3);

  glfwSwapBuffers(window);
}

void processInput(GLFWwindow *window) {
  if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS) {
    glfwSetWindowShouldClose(window, true);
  }
}

void renderLoop(GLFWwindow *window) {
  while (!glfwWindowShouldClose(window)) {
    processInput(window);
    renderScene(window);
    glfwPollEvents();
  }
}

void shutdown(GLFWwindow *) {
  glDeleteBuffers(1, &VBO);
  glDeleteVertexArrays(1, &VAO);
  glDeleteProgram(program);
}
