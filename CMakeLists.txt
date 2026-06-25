cmake_minimum_required(VERSION 3.20)
project(underwater-computer-graphics LANGUAGES CXX)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})

find_package(OpenGL REQUIRED)
find_package(glfw3  REQUIRED)
find_package(GLEW   REQUIRED)
find_package(assimp REQUIRED)
find_package(glm    REQUIRED)

add_executable(underwater-computer-graphics src/main.cpp)
add_executable(perlin src/PerlinNoiseExample.cpp)

# IDE-only target: scene.cpp is included from main.cpp, but clangd needs its own
# compile_commands entry with the correct include paths.
add_library(scene_ide OBJECT src/scene.cpp)
target_link_libraries(scene_ide PRIVATE
    OpenGL::GL
    glfw
    GLEW::GLEW
    assimp::assimp
    glm::glm)

target_link_libraries(underwater-computer-graphics PRIVATE
    OpenGL::GL
    glfw
    GLEW::GLEW
    assimp::assimp
    glm::glm)

if(APPLE)
    target_link_libraries(scene_ide PRIVATE
        "-framework CoreGraphics"
        "-framework ImageIO"
        "-framework CoreFoundation")
    target_link_libraries(underwater-computer-graphics PRIVATE
        "-framework CoreGraphics"
        "-framework ImageIO"
        "-framework CoreFoundation")
endif()

target_link_libraries(perlin PRIVATE glm::glm)

# Copy shaders/img/models whenever any file in those directories changes.
# Implemented as a separate custom target so ninja actually re-runs the copy
# even when no C++ source has been touched.
file(GLOB_RECURSE SHADER_FILES CONFIGURE_DEPENDS ${CMAKE_SOURCE_DIR}/shaders/*)
file(GLOB_RECURSE IMG_FILES     CONFIGURE_DEPENDS ${CMAKE_SOURCE_DIR}/img/*)
file(GLOB_RECURSE MODEL_FILES   CONFIGURE_DEPENDS ${CMAKE_SOURCE_DIR}/models/*)

add_custom_target(copy_assets ALL
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_SOURCE_DIR}/shaders $<TARGET_FILE_DIR:underwater-computer-graphics>/shaders
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_SOURCE_DIR}/img     $<TARGET_FILE_DIR:underwater-computer-graphics>/img
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_SOURCE_DIR}/models  $<TARGET_FILE_DIR:underwater-computer-graphics>/models
    DEPENDS ${SHADER_FILES} ${IMG_FILES} ${MODEL_FILES}
    VERBATIM)

add_dependencies(underwater-computer-graphics copy_assets)
