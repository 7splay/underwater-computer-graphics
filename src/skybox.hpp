#pragma once

#include <GL/glew.h>

#include <glm/glm.hpp>

#include <vector>

#include "renderable.hpp"
#include "underwater_atmosphere.hpp"

// Procedural underwater sky color for a given world-space direction.
inline glm::vec3 skyColorFromDir(const glm::vec3 &dir) {
    const glm::vec3 abyss{0.005f, 0.02f, 0.05f};
    const glm::vec3 surface{0.14f, 0.38f, 0.48f};

    float t = glm::clamp(dir.y * 0.5f + 0.5f, 0.0f, 1.0f);
    t = t * t * (3.0f - 2.0f * t);  // smoothstep
    glm::vec3 color = glm::mix(abyss, surface, t);

    // soft sun glow baked in: subtle so the sky reads naturally underwater
    const glm::vec3 sunDir = glm::normalize(underwater::kSunDirection);
    float cosA = glm::max(glm::dot(glm::normalize(dir), sunDir), 0.0f);
    float glow = glm::pow(cosA, 8.0f) * 0.10f;
    color += underwater::kSunColor * glow;
    return color;
}

// Procedural cubemap baked from skyColorFromDir. SEAMLESS filtering must be
// enabled at startup so OpenGL blends across cube face edges.
inline GLuint generateSkyboxCubemap(int size = 256) {
    GLuint tex = 0;
    glGenTextures(1, &tex);
    glBindTexture(GL_TEXTURE_CUBE_MAP, tex);

    std::vector<unsigned char> data(static_cast<std::size_t>(size) * size * 3);
    for (int face = 0; face < 6; ++face) {
        for (int py = 0; py < size; ++py) {
            for (int px = 0; px < size; ++px) {
                float sc = 2.0f * (px + 0.5f) / size - 1.0f;
                float tc = 2.0f * (py + 0.5f) / size - 1.0f;
                glm::vec3 dir;
                switch (face) {
                    case 0: dir = glm::vec3( 1.0f, -tc, -sc); break;  // +X
                    case 1: dir = glm::vec3(-1.0f, -tc,  sc); break;  // -X
                    case 2: dir = glm::vec3( sc,  1.0f,  tc); break;  // +Y
                    case 3: dir = glm::vec3( sc, -1.0f, -tc); break;  // -Y
                    case 4: dir = glm::vec3( sc, -tc,  1.0f); break;  // +Z
                    case 5: dir = glm::vec3(-sc, -tc, -1.0f); break;  // -Z
                }
                glm::vec3 color = skyColorFromDir(dir);
                auto i = static_cast<std::size_t>((py * size + px) * 3);
                data[i + 0] = static_cast<unsigned char>(color.r * 255.0f);
                data[i + 1] = static_cast<unsigned char>(color.g * 255.0f);
                data[i + 2] = static_cast<unsigned char>(color.b * 255.0f);
            }
        }
        glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + face, 0, GL_RGB, size,
                     size, 0, GL_RGB, GL_UNSIGNED_BYTE, data.data());
    }

    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
    glBindTexture(GL_TEXTURE_CUBE_MAP, 0);
    return tex;
}

// Unit cube centered on origin, positions only. Rendered with the depth=1
// trick so size is irrelevant. The cube's local positions double as the
// cubemap sample direction.
inline Renderable makeSkybox() {
    static const float cubeVerts[] = {
        -1.0f, -1.0f, -1.0f,  // 0
         1.0f, -1.0f, -1.0f,  // 1
         1.0f,  1.0f, -1.0f,  // 2
        -1.0f,  1.0f, -1.0f,  // 3
        -1.0f, -1.0f,  1.0f,  // 4
         1.0f, -1.0f,  1.0f,  // 5
         1.0f,  1.0f,  1.0f,  // 6
        -1.0f,  1.0f,  1.0f,  // 7
    };
    static const unsigned int cubeIndices[] = {
        0, 2, 1,  0, 3, 2,  // -Z
        4, 5, 6,  4, 6, 7,  // +Z
        0, 4, 7,  0, 7, 3,  // -X
        1, 2, 6,  1, 6, 5,  // +X
        0, 1, 5,  0, 5, 4,  // -Y
        3, 7, 6,  3, 6, 2,  // +Y
    };

    Renderable skybox{};
    skybox.indexCount = 36;

    glGenVertexArrays(1, &skybox.VAO);
    glGenBuffers(1, &skybox.VBO);
    glGenBuffers(1, &skybox.EBO);

    glBindVertexArray(skybox.VAO);

    glBindBuffer(GL_ARRAY_BUFFER, skybox.VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(cubeVerts), cubeVerts, GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, skybox.EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(cubeIndices), cubeIndices,
                 GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float),
                          reinterpret_cast<void *>(0));
    glEnableVertexAttribArray(0);

    glBindVertexArray(0);
    return skybox;
}
