//#ifndef Mesh_hpp
//#define Mesh_hpp
//
//#if defined (__APPLE__)
//#define GL_SILENCE_DEPRECATION
//#include <OpenGL/gl3.h>
//#else
//#define GLEW_STATIC
//#include "GL/glew.h"
//#endif
//
//#include "glm/glm.hpp"
//
//#include "Shader.hpp"
//
//#include <string>
//#include <vector>
//
//
//namespace gps {
//
//    struct Vertex {
//
//        glm::vec3 Position;
//        glm::vec3 Normal;
//        glm::vec2 TexCoords;
//		glm::vec3 Tangent;
//		glm::vec3 Bitangent;
//    };
//
//    struct Texture {
//
//        GLuint id;
//        //ambientTexture, diffuseTexture, specularTexture
//        std::string type;
//        std::string path;
//    };
//
//    struct Material {
//        glm::vec1 specular_highlight;
//        glm::vec3 ambient;
//        glm::vec3 diffuse;
//        glm::vec3 specular;
//        glm::vec3 emissive;
//        glm::vec1 refract_index;
//        glm::vec1 dissolve;
//        glm::vec1 illum;
//    };
//
//    struct Buffers {
//        GLuint VAO;
//        GLuint VBO;
//        GLuint EBO;
//    };
//
//    class Mesh {
//
//    public:
//        std::vector<Vertex> vertices;
//        std::vector<GLuint> indices;
//        std::vector<Texture> textures;
//
//        Mesh(std::vector<Vertex> vertices, std::vector<GLuint> indices, std::vector<Texture> textures);
//
//        Buffers getBuffers();
//
//        void Draw(gps::Shader shader);
//
//    private:
//        /*  Render data  */
//        Buffers buffers;
//
//        // Initializes all the buffer objects/arrays
//        void setupMesh();
//
//    };
//
//}
//#endif /* Mesh_hpp */
