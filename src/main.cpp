#include <GL/glew.h>
#include <GLFW/glfw3.h>

#include <cstdlib>
#include <iostream>

#include "scene.cpp"

int main(int argc, char **argv) {
  // optional object-density entry parameter: ./underwater-computer-graphics [density]
  // higher packs in more corals/seaweed/fish, lower thins the scene out
  if (argc > 1) {
    const float density = static_cast<float>(std::atof(argv[1]));
    if (density > 0.0f) {
      objectDensity = density;
    }
  }

  if (!glfwInit()) {
    std::cerr << "Failed to initialize GLFW" << std::endl;
    return -1;
  }

  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
  glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
#ifdef __APPLE__
  glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

  GLFWwindow *window =
      glfwCreateWindow(800, 600, "Underwater Scene", nullptr, nullptr);
  if (window == nullptr) {
    std::cerr << "Failed to create GLFW window" << std::endl;
    glfwTerminate();
    return -1;
  }

  glfwMakeContextCurrent(window);

  glewExperimental = GL_TRUE;
  if (glewInit() != GLEW_OK) {
    std::cerr << "Failed to initialize GLEW" << std::endl;
    glfwDestroyWindow(window);
    glfwTerminate();
    return -1;
  }

  // VSync: cap to display refresh rate. gives a stable 60 FPS, no tearing, and
  // lets the GPU idle between frames instead of running flat-out for nothing
  glfwSwapInterval(1);

  init(window);
  renderLoop(window);
  shutdown(window);

  glfwDestroyWindow(window);
  glfwTerminate();
  return 0;
}