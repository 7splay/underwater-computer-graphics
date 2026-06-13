#pragma once

#include "PerlinNoise.hpp"
#include "models.hpp"

inline siv::PerlinNoise noise { 42u };

// grid[z][x] in [0, 1], dimensions (resZ+1) x (resX+1)
// higher smoothness -> lower noise frequency -> wider, gentler dunes
inline std::vector<std::vector<float>> generateHeightMap(int resX, int resZ,
                                                         float smoothness)
{
    std::vector<std::vector<float>> grid(resZ + 1, std::vector<float>(resX + 1));
    for (int z = 0; z <= resZ; ++z) {
        for (int x = 0; x <= resX; ++x) {
            grid[z][x] = noise.octave2D_01(x / smoothness, z / smoothness, 3);
        }
    }
    return grid;
}

inline Renderable generateSand(int resX, int resZ, float gap, float amplitude, float smoothness, glm::mat4 model=glm::mat4(1.0f))
{
    std::vector<std::vector<float>> heightMap = generateHeightMap(resX, resZ, smoothness);

    std::vector<float> vertices;
    vertices.reserve((resX + 1) * (resZ + 1) * 3);
    for (int z = 0; z <= resZ; ++z) {
        for (int x = 0; x <= resX; ++x) {
            vertices.push_back((x - resX * 0.5f) * gap);
            vertices.push_back(heightMap[z][x] * amplitude - amplitude * 0.5f);
            vertices.push_back((z - resZ * 0.5f) * gap);
        }
    }

    std::vector<unsigned int> indices;
    indices.reserve(resX * resZ * 6);
    for (int z = 0; z < resZ; ++z) {
        for (int x = 0; x < resX; ++x) {
            unsigned int topLeft = z * (resX + 1) + x;
            unsigned int topRight = topLeft + 1;
            unsigned int bottomLeft = (z + 1) * (resX + 1) + x;
            unsigned int bottomRight = bottomLeft + 1;

            indices.push_back(topLeft);
            indices.push_back(bottomLeft);
            indices.push_back(topRight);

            indices.push_back(topRight);
            indices.push_back(bottomLeft);
            indices.push_back(bottomRight);
        }
    }

    Renderable sand {};
    glGenVertexArrays(1, &sand.VAO);
    glGenBuffers(1, &sand.VBO);
    glGenBuffers(1, &sand.EBO);

    glBindVertexArray(sand.VAO);

    glBindBuffer(GL_ARRAY_BUFFER, sand.VBO);
    glBufferData(GL_ARRAY_BUFFER, static_cast<GLsizeiptr>(vertices.size() * sizeof(float)), vertices.data(), GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, sand.EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER,
        static_cast<GLsizeiptr>(indices.size() * sizeof(unsigned int)),
        indices.data(), GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(float) * 3, (void *)0);
    glEnableVertexAttribArray(0);

    glBindVertexArray(0);
    
    sand.indexCount = static_cast<GLsizei>(indices.size());
    sand.model = model;
    return sand;
}