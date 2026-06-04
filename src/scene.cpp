#pragma once

#include <GL/glew.h>
#include <GLFW/glfw3.h>

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <vector>

#include "models.hpp"
#include "shader.hpp"

GLuint program = 0;
std::vector<Renderable> renderables;
GLint viewLocation = -1;
GLint projLocation = -1;
GLint modelLocation = -1;
GLint textureLocation = -1;

void framebuffer_size_callback(GLFWwindow *, int width, int height) {
  glViewport(0, 0, width, height);
}

void initModel() {
  glEnable(GL_DEPTH_TEST);

  program = createProgram("shaders/model.vert", "shaders/model.frag");
  renderables = loadSceneModels();

  viewLocation = glGetUniformLocation(program, "uView");
  projLocation = glGetUniformLocation(program, "uProjection");
  modelLocation = glGetUniformLocation(program, "uModel");
  textureLocation = glGetUniformLocation(program, "uTexture");

  if (textureLocation >= 0) {
    glUseProgram(program);
    glUniform1i(textureLocation, 0);
  }
}

void init(GLFWwindow *window) {
  glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
  glViewport(0, 0, 800, 600);

  initModel();
}

void renderScene(GLFWwindow *window) {
  glClearColor(0.05f, 0.15f, 0.25f, 1.0f);
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

  int width = 0;
  int height = 0;
  glfwGetFramebufferSize(window, &width, &height);
  if (height == 0) {
    height = 1;
  }

  glUseProgram(program);

  glm::mat4 view =
      glm::lookAt(glm::vec3(0.0f, 1.5f, 6.0f), glm::vec3(0.0f, 0.0f, 0.0f),
                  glm::vec3(0.0f, 1.0f, 0.0f));
  glm::mat4 projection = glm::perspective(
      glm::radians(35.0f),
      static_cast<float>(width) / static_cast<float>(height), 0.1f, 100.0f);

  if (viewLocation >= 0) {
    glUniformMatrix4fv(viewLocation, 1, GL_FALSE, glm::value_ptr(view));
  }
  if (projLocation >= 0) {
    glUniformMatrix4fv(projLocation, 1, GL_FALSE, glm::value_ptr(projection));
  }

  for (const Renderable &renderable : renderables) {
    if (modelLocation >= 0) {
      glUniformMatrix4fv(modelLocation, 1, GL_FALSE,
                         glm::value_ptr(renderable.model));
    }

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, renderable.texture);
    glBindVertexArray(renderable.VAO);
    glDrawElements(GL_TRIANGLES, renderable.indexCount, GL_UNSIGNED_INT,
                   nullptr);
  }

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
  destroyRenderables(renderables);
  deleteProgram(program);
}
