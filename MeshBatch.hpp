// MeshBatch.hpp

#ifndef MeshBatch_hpp
#define MeshBatch_hpp

#include "Shader.hpp"
#include "glm/glm.hpp"
#include "Frustum.hpp"

#include <vector>
#include <string>
#include <unordered_map>

namespace gps {
    struct Vertex {
        glm::vec3 Position;
        glm::vec3 Normal;
        glm::vec2 TexCoords;
        glm::vec3 Tangent;
        glm::vec3 Bitangent;

        bool operator==(const Vertex& other) const {
            return Position == other.Position &&
                Normal == other.Normal &&
                TexCoords == other.TexCoords &&
                Tangent == other.Tangent &&
                Bitangent == other.Bitangent;
        }
    };

    struct VertexHash {
        size_t operator()(const Vertex& vertex) const {
            return std::hash<float>()(vertex.Position.x) ^
                (std::hash<float>()(vertex.Position.y) << 1) ^
                (std::hash<float>()(vertex.Position.z) << 2) ^
                (std::hash<float>()(vertex.Normal.x) << 3) ^
                (std::hash<float>()(vertex.Normal.y) << 4) ^
                (std::hash<float>()(vertex.Normal.z) << 5) ^
                (std::hash<float>()(vertex.TexCoords.x) << 6) ^
                (std::hash<float>()(vertex.TexCoords.y) << 7) ^
                (std::hash<float>()(vertex.Tangent.x) << 8) ^
                (std::hash<float>()(vertex.Tangent.y) << 9) ^
                (std::hash<float>()(vertex.Tangent.z) << 10) ^
                (std::hash<float>()(vertex.Bitangent.x) << 11) ^
                (std::hash<float>()(vertex.Bitangent.y) << 12) ^
                (std::hash<float>()(vertex.Bitangent.z) << 13);
        }
    };

    struct Texture {
        GLuint id;
        std::string type;
        std::string path;

        bool operator==(const Texture& other) const {
            return id == other.id && type == other.type && path == other.path;
        }
    };

    struct MeshBatch {
        std::vector<Vertex> vertices;
        std::vector<GLuint> indices;
        std::vector<Texture> textures;
        bool isRockMaterial;
        bool isWindMovable;
        bool isGrass;
        bool isFern;
        GLuint VAO, VBO, EBO;
        GLuint indexCount;

        // Bounding box for frustum culling
        glm::vec3 minBounds;
        glm::vec3 maxBounds;

        MeshBatch()
            : vertices(),
            indices(),
            textures(),
            isRockMaterial(false),
            isWindMovable(false),
            isGrass(false),
            isFern(false),
            VAO(0),
            VBO(0),
            EBO(0),
            indexCount(0),
            minBounds(glm::vec3(0.0f)),
            maxBounds(glm::vec3(0.0f)) {}

        void setupBuffers() {
            glGenVertexArrays(1, &VAO);
            glGenBuffers(1, &VBO);
            glGenBuffers(1, &EBO);

            glBindVertexArray(VAO);

            glBindBuffer(GL_ARRAY_BUFFER, VBO);
            glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(Vertex), &vertices[0], GL_STATIC_DRAW);

            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
            glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(GLuint), &indices[0], GL_STATIC_DRAW);

            glEnableVertexAttribArray(0);
            glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (GLvoid*)0);

            glEnableVertexAttribArray(1);
            glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (GLvoid*)offsetof(Vertex, Normal));

            glEnableVertexAttribArray(2);
            glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (GLvoid*)offsetof(Vertex, TexCoords));

            glEnableVertexAttribArray(3);
            glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (GLvoid*)offsetof(Vertex, Tangent));

            glEnableVertexAttribArray(4);
            glVertexAttribPointer(4, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (GLvoid*)offsetof(Vertex, Bitangent));

            glBindVertexArray(0);

            calculateBounds();
        }

        void calculateBounds() {
            if (vertices.empty()) return;

            minBounds = vertices[0].Position;
            maxBounds = vertices[0].Position;

            for (const auto& vertex : vertices) {
                minBounds = glm::min(minBounds, vertex.Position);
                maxBounds = glm::max(maxBounds, vertex.Position);
            }
        }

        void Draw(Shader shader, const Frustum& frustum) {
            if (!frustum.isVisible(minBounds, maxBounds))
                return;

            shader.useShaderProgram();

            if (shader.shaderType == MAIN_SHADER || shader.shaderType == SHADOW_SHADER) {
                GLint objectTypeLoc = shader.getUniformLocation("u_ObjectType");
                if (objectTypeLoc != -1) {
                    glUniform1i(objectTypeLoc, (isGrass ? 0 : (isFern ? 2 : 1)));
                }
                GLint isWindMovableLoc = shader.getUniformLocation("isWindMovable");
                if (isWindMovableLoc != -1) {
                    glUniform1i(isWindMovableLoc, isWindMovable ? 1 : 0);
                }
            }

            if (shader.shaderType == MAIN_SHADER) {

                GLint useBlinnLoc = shader.getUniformLocation("useBlinnPhong");
                if (useBlinnLoc != -1) {
                    glUniform1i(useBlinnLoc, isRockMaterial ? 1 : 0);
                }

                // Conditionally bind and set texture uniforms
                for (GLuint i = 0; i < textures.size(); i++) {
                    glActiveTexture(GL_TEXTURE0 + i);
                    GLint uniformLoc = shader.getUniformLocation(textures[i].type.c_str());
                    if (uniformLoc != -1) {
                        glUniform1i(uniformLoc, i);
                        glBindTexture(GL_TEXTURE_2D, textures[i].id);
                    }
                }
            }
            // For other shader types, skip setting uniforms not relevant
            // You can add more else-if blocks for other shader types if needed

            // Draw the mesh
            glBindVertexArray(VAO);
            glDrawElements(GL_TRIANGLES, indexCount, GL_UNSIGNED_INT, 0);
            glBindVertexArray(0);

            // Unbind textures if they were bound
            if (shader.shaderType == MAIN_SHADER) {
                for (GLuint i = 0; i < textures.size(); i++) {
                    glActiveTexture(GL_TEXTURE0 + i);
                    glBindTexture(GL_TEXTURE_2D, 0);
                }
            }
        }



        void Cleanup() {
            glDeleteVertexArrays(1, &VAO);
            glDeleteBuffers(1, &VBO);
            glDeleteBuffers(1, &EBO);
        }
    };

}

#endif
