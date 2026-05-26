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

file(GLOB_RECURSE PROJECT_SOURCES CONFIGURE_DEPENDS
     "${CMAKE_SOURCE_DIR}/src/*.cpp")

add_executable(underwater-computer-graphics ${PROJECT_SOURCES})

target_link_libraries(underwater-computer-graphics PRIVATE
    OpenGL::GL
    glfw
    GLEW::GLEW
    assimp::assimp
    glm::glm)

add_custom_command(TARGET underwater-computer-graphics POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_SOURCE_DIR}/shaders $<TARGET_FILE_DIR:underwater-computer-graphics>/shaders)

add_custom_command(TARGET underwater-computer-graphics POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_SOURCE_DIR}/img $<TARGET_FILE_DIR:underwater-computer-graphics>/img)

add_custom_command(TARGET underwater-computer-graphics POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_SOURCE_DIR}/models $<TARGET_FILE_DIR:underwater-computer-graphics>/models)