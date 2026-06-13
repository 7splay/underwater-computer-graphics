#include <GL/glew.h>
#include <GLFW/glfw3.h>

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <cmath>
#include <vector>

#include "camera.hpp"
#include "models.hpp"
#include "shader.hpp"
#include "seabed.hpp"

int width = 800;
int height = 600;

GLuint program = 0;
std::vector<Renderable> renderables;

GLint viewLocation = -1;
GLint projLocation = -1;
GLint modelLocation = -1;
GLint textureLocation = -1;

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

void framebuffer_size_callback(GLFWwindow *, int width, int height) {
  glViewport(0, 0, width, height);
}

void initModel() {
  glEnable(GL_DEPTH_TEST);

  program = createProgram("shaders/model.vert", "shaders/model.frag");
  renderables = loadSceneModels();
  renderables.push_back(generateSand(128, 128, sandDensity, sandAmplitude, sandSmoothness, sandModel));

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
#ifdef __APPLE__
  glViewport(0, 0, width * 2, height * 2); // account for retina display
#else
  glViewport(0, 0, width, height);
#endif

  initModel();
  glfwSetCursorPosCallback(window, processMouse);
  glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
}

void renderScene(GLFWwindow *window) {
  glClearColor(0.05f, 0.15f, 0.25f, 1.0f);
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

  glfwGetFramebufferSize(window, &width, &height);
  if (height == 0) {
    height = 1;
  }
  glUseProgram(program);

  glm::mat4 view = getViewMatrix();
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

  glm::vec3 inputDirection = glm::vec3(0.0f);
  glm::vec3 independentInputDirection = glm::vec3(0.0f);

  if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) {
    inputDirection += glm::vec3(0.0f, 0.0f, -1.0f);
  }
  if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) {
    inputDirection += glm::vec3(0.0f, 0.0f, 1.0f);
  }
  if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) {
    inputDirection += glm::vec3(-1.0f, 0.0f, 0.0f);
  }
  if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) {
    inputDirection += glm::vec3(1.0f, 0.0f, 0.0f);
  }
  if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS) {
    independentInputDirection += glm::vec3(0.0f, 1.0f, 0.0f);
  }
  if (glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS) {
    independentInputDirection += glm::vec3(0.0f, -1.0f, 0.0f);
  }
  updateCameraMovement(inputDirection, independentInputDirection, cameraSpeed,
                       deltaTime);
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
  while (!glfwWindowShouldClose(window)) {
    float time = static_cast<float>(glfwGetTime());
    deltaTime = time - lastTime;
    lastTime = time;

    processInput(window);
    renderScene(window);
    glfwPollEvents();
  }
}

void shutdown(GLFWwindow *) {
  destroyRenderables(renderables);
  deleteProgram(program);
}
