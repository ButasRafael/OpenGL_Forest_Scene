// Model3D.hpp
#ifndef Model3D_hpp
#define Model3D_hpp

#include "Mesh.hpp"
#include "MeshBatch.hpp"
#include "tiny_obj_loader.h"
#include "stb_image.h"

#include <iostream>
#include <string>
#include <vector>
#include <map>

namespace gps {

    class Model3D {

    public:
        ~Model3D();

        std::vector<gps::MeshBatch> meshBatches;

        void LoadModel(std::string fileName);

        void LoadModel(std::string fileName, std::string basePath);

		void Draw(gps::Shader shaderProgram, const glm::mat4& viewMatrix, const glm::mat4& projectionMatrix);

    private:
        std::vector<gps::Texture> loadedTextures;

        void ReadOBJ(std::string fileName, std::string basePath);
        gps::Texture LoadTexture(std::string path, std::string type);
        GLuint ReadTextureFromFile(const char* file_name);
    };

}

#endif