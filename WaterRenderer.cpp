#include "WaterRenderer.hpp"
#include "glm/gtc/type_ptr.hpp"
#include <iostream>
#include <cmath>

WaterRenderer::WaterRenderer(const std::string& vertexShaderPath,
    const std::string& fragmentShaderPath,
    const glm::mat4& projectionMatrix, const std::string& dudvMapPath, const std::string& normalMapPath)
{
    waterShader.loadShader(vertexShaderPath, fragmentShaderPath, gps::ShaderType::WATER_SHADER);
    setupWaterQuad();

    waterShader.useShaderProgram();
    glUniformMatrix4fv(glGetUniformLocation(waterShader.shaderProgram, "projection"), 1, GL_FALSE, glm::value_ptr(projectionMatrix));

    dudvMap = loadTexture(dudvMapPath);
	normalMap = loadTexture(normalMapPath);
}

WaterRenderer::~WaterRenderer()
{
    glDeleteVertexArrays(1, &VAO);
    glDeleteBuffers(1, &VBO);
    glDeleteBuffers(1, &EBO);
    glDeleteTextures(1, &dudvMap);
	glDeleteTextures(1, &normalMap);
}

GLuint WaterRenderer::loadTexture(const std::string& filepath) {
    int width, height, nrChannels;
    stbi_set_flip_vertically_on_load(true);
    unsigned char* data = stbi_load(filepath.c_str(), &width, &height, &nrChannels, 0);
    if (!data) {
        std::cerr << "Failed to load texture: " << filepath << std::endl;
        return 0;
    }

    GLuint textureID;
    glGenTextures(1, &textureID);
    glBindTexture(GL_TEXTURE_2D, textureID);

    GLenum format;
    if (nrChannels == 1)
        format = GL_RED;
    else if (nrChannels == 3)
        format = GL_RGB;
    else if (nrChannels == 4)
        format = GL_RGBA;

    glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
    glGenerateMipmap(GL_TEXTURE_2D);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    stbi_image_free(data);

    return textureID;
}

void WaterRenderer::setupWaterQuad()
{
    float vertices[] = {
        -1.0f, -1.0f,     0.0f, 0.0f,
         1.0f, -1.0f,     1.0f, 0.0f,
        -1.0f,  1.0f,     0.0f, 1.0f,
         1.0f,  1.0f,     1.0f, 1.0f,
    };

    unsigned int indices[] = {
        0, 2, 1,
        1, 2, 3
    };

    glGenVertexArrays(1, &VAO);
    glBindVertexArray(VAO);

    glGenBuffers(1, &VBO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    glGenBuffers(1, &EBO);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);

    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));

    glBindVertexArray(0);
}


void WaterRenderer::render(const std::vector<WaterTile>& waterTiles, const glm::mat4& viewMatrix,
	const glm::vec3& cameraPosition, GLuint reflectionTexture, GLuint refractionTexture, GLuint depthTexture,
    double deltaTime, const glm::vec3& lightPosition, const glm::vec3& lightColor)
{
    waterShader.useShaderProgram();

    glBindVertexArray(VAO);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, reflectionTexture);
    glUniform1i(glGetUniformLocation(waterShader.shaderProgram, "reflectionTexture"), 0);

    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, refractionTexture);
    glUniform1i(glGetUniformLocation(waterShader.shaderProgram, "refractionTexture"), 1);

    glActiveTexture(GL_TEXTURE2);
    glBindTexture(GL_TEXTURE_2D, dudvMap);
    glUniform1i(glGetUniformLocation(waterShader.shaderProgram, "dudvMap"), 2);

    glActiveTexture(GL_TEXTURE3);
    glBindTexture(GL_TEXTURE_2D, normalMap);
    glUniform1i(glGetUniformLocation(waterShader.shaderProgram, "normalMap"), 3);

	glActiveTexture(GL_TEXTURE4);
	glBindTexture(GL_TEXTURE_2D, depthTexture);
	glUniform1i(glGetUniformLocation(waterShader.shaderProgram, "depthMap"), 4);

    moveFactor += waveSpeed * deltaTime;
    moveFactor = fmod(moveFactor, 1.0f);

    glUniform1f(glGetUniformLocation(waterShader.shaderProgram, "moveFactor"), moveFactor);

    glUniform3fv(glGetUniformLocation(waterShader.shaderProgram, "cameraPos"), 1, glm::value_ptr(cameraPosition));

    glUniform3fv(glGetUniformLocation(waterShader.shaderProgram, "lightPosition"), 1, glm::value_ptr(lightPosition));

    glUniform3fv(glGetUniformLocation(waterShader.shaderProgram, "lightColor"), 1, glm::value_ptr(lightColor));

    for (const auto& tile : waterTiles) {
        glm::mat4 model = glm::mat4(1.0f);
        model = glm::translate(model, glm::vec3(tile.x, tile.height, tile.z));
        model = glm::scale(model, glm::vec3(WaterTile::TILE_SIZE));

        glUniformMatrix4fv(glGetUniformLocation(waterShader.shaderProgram, "model"), 1, GL_FALSE, glm::value_ptr(model));
        glUniformMatrix4fv(glGetUniformLocation(waterShader.shaderProgram, "view"), 1, GL_FALSE, glm::value_ptr(viewMatrix));

        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
    }

    glDisable(GL_BLEND);
    glBindVertexArray(0);
}

