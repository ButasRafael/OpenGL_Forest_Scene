#ifndef SKYBOX_HPP
#define SKYBOX_HPP

#include <vector>
#include <string>
#include "Shader.hpp"

class Skybox {
public:
    Skybox(const std::vector<std::string>& faces);
    void Draw(gps::Shader& shader);
    GLuint getCubemapTexture() const { return cubemapTexture; }

private:
    GLuint cubemapTexture;
    GLuint VAO, VBO;

    GLuint loadCubemap(const std::vector<std::string>& faces);


    void setupMesh();
};

#endif