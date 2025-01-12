#ifndef WATER_RENDERER_HPP
#define WATER_RENDERER_HPP

#include <vector>
#include "glm/glm.hpp"
#include "Shader.hpp"
#include "WaterTile.hpp"
#include "stb_image.h"

class WaterRenderer {
public:
    WaterRenderer(const std::string& vertexShaderPath,
        const std::string& fragmentShaderPath,
        const glm::mat4& projectionMatrix, const std::string& dudvMapPath, const std::string& normalMapPath);
    ~WaterRenderer();

    void render(const std::vector<WaterTile>& waterTiles, const glm::mat4& viewMatrix,
		const glm::vec3& cameraPosition, GLuint reflectionTexture, GLuint refractionTexture, GLuint depthTexture,
        double deltaTime, const glm::vec3& lightPosition, const glm::vec3& lightColor);

    GLuint loadTexture(const std::string& filepath);

private:
    void setupWaterQuad();

    gps::Shader waterShader;
    unsigned int VAO, VBO, EBO;
    GLuint dudvMap;
	GLuint normalMap;

    float waveSpeed = 0.03f;
    float moveFactor = 0.0f;
};

#endif
