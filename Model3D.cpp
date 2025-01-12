// Model3D.cpp

#include "Model3D.hpp"
#include "Frustum.hpp" // Include the Frustum class
#include "BVH.hpp"
#include <algorithm>
#include <unordered_map>
#include <mutex>
#include "meshoptimizer.h"

namespace gps {

    const size_t MAX_BATCH_SIZE = 35000;
    std::vector<bool> meshMaterials;
    std::mutex meshBatchesMutex;
    gps::BVH bvh;

    bool compareMeshBatches(const gps::MeshBatch& a, const gps::MeshBatch& b) {
        if (a.textures.empty() && b.textures.empty()) return false;
        if (a.textures.empty()) return true;
        if (b.textures.empty()) return false;
        return a.textures[0].id < b.textures[0].id;
    }

    void Model3D::LoadModel(std::string fileName) {
        std::string basePath = fileName.substr(0, fileName.find_last_of('/')) + "/";
        ReadOBJ(fileName, basePath);
    }

    void Model3D::LoadModel(std::string fileName, std::string basePath) {
        ReadOBJ(fileName, basePath);
    }

    std::vector<MeshBatch> SplitBatch(const MeshBatch& originalBatch) {
        std::vector<MeshBatch> subBatches;
        size_t totalIndices = originalBatch.indices.size();
        size_t indicesPerBatch = (MAX_BATCH_SIZE / 3) * 3; // Ensure divisible by 3 for triangles

        size_t startIndex = 0;
        while (startIndex < totalIndices) {
            size_t endIndex = std::min(startIndex + indicesPerBatch, totalIndices);
            MeshBatch subBatch;
            subBatch.textures = originalBatch.textures;
            subBatch.isRockMaterial = originalBatch.isRockMaterial;
            subBatch.isWindMovable = originalBatch.isWindMovable;
			subBatch.isGrass = originalBatch.isGrass;
			subBatch.isFern = originalBatch.isFern;

            // Copy the relevant vertices and indices
            std::vector<Vertex> subVertices;
            std::vector<GLuint> subIndices;

            // To avoid duplicating vertices, maintain a map from original vertex index to sub-batch vertex index
            std::unordered_map<GLuint, GLuint> vertexMapping;
            GLuint newIndex = 0;

            for (size_t i = startIndex; i < endIndex; ++i) {
                GLuint originalIdx = originalBatch.indices[i];

                // If the vertex hasn't been copied yet, copy it to subVertices
                if (vertexMapping.find(originalIdx) == vertexMapping.end()) {
                    subVertices.push_back(originalBatch.vertices[originalIdx]);
                    vertexMapping[originalIdx] = newIndex++;
                }

                // Add the new index to subIndices
                subIndices.push_back(vertexMapping[originalIdx]);
            }

            subBatch.vertices = subVertices;
            subBatch.indices = subIndices;
            subBatch.indexCount = static_cast<GLuint>(subIndices.size());

            subBatches.push_back(subBatch);
            startIndex += indicesPerBatch;
        }

        return subBatches;
    }


    // Updated Draw method to accept view and projection matrices
    void Model3D::Draw(gps::Shader shaderProgram, const glm::mat4& viewMatrix, const glm::mat4& projectionMatrix) {
        // Create and update the frustum
        Frustum frustum;
        frustum.update(viewMatrix, projectionMatrix);

        //for (auto& batch : meshBatches) {
        //    batch.Draw(shaderProgram, frustum);
        //}
		// Draw the BVH
		bvh.frustumCulledDraw(frustum, shaderProgram);
    }

	void OptimizeMesh(int MeshIndex, std::vector <Vertex>& vertices, std::vector<GLuint>& indices) {
		size_t vertexCount = vertices.size();
		size_t indexCount = indices.size();
		std::vector <unsigned int> remap(indexCount);
		size_t OptVertexCount = meshopt_generateVertexRemap(remap.data(), indices.data(), indexCount, vertices.data(), vertexCount, sizeof(Vertex));
        std::vector <Vertex> OptVertices;
		OptVertices.resize(OptVertexCount);
		std::vector <GLuint> OptIndices;
		OptIndices.resize(indexCount);
		meshopt_remapIndexBuffer(OptIndices.data(), indices.data(), indexCount, remap.data());
		meshopt_remapVertexBuffer(OptVertices.data(), vertices.data(), vertexCount, sizeof(Vertex), remap.data());
		meshopt_optimizeVertexCache(OptIndices.data(), OptIndices.data(), indexCount, OptVertexCount);;
		meshopt_optimizeVertexFetch(OptVertices.data(), OptIndices.data(), indexCount, OptVertices.data(), OptVertexCount, sizeof(Vertex));
        float threshold = 1.0f;
		size_t targetIndexCount = (size_t)(indexCount * threshold);
        float targetError = 1e-2f;
		std::vector <unsigned int> simplifiedIndices(OptIndices.size());
		size_t simplifiedIndexCount = meshopt_simplify(simplifiedIndices.data(), OptIndices.data(), indexCount, &OptVertices[0].Position.x, OptVertexCount, sizeof(Vertex), targetIndexCount, targetError);
		simplifiedIndices.resize(simplifiedIndexCount);

        indices = simplifiedIndices;
		vertices = OptVertices;

	}   

    void Model3D::ReadOBJ(std::string fileName, std::string basePath) {

        std::cout << "Loading : " << fileName << std::endl;
        tinyobj::attrib_t attrib;
        std::vector<tinyobj::shape_t> shapes;
        std::vector<tinyobj::material_t> materials;
        int materialId;

        std::string err;
        bool ret = tinyobj::LoadObj(&attrib, &shapes, &materials, &err, fileName.c_str(), basePath.c_str(), true);

        if (!err.empty()) {
            std::cerr << err << std::endl;
        }

        if (!ret) {
            exit(1);
        }

        std::cout << "# of shapes    : " << shapes.size() << std::endl;
        std::cout << "# of materials : " << materials.size() << std::endl;

        meshMaterials.clear();
        std::map<int, gps::MeshBatch> batches;

        for (size_t s = 0; s < shapes.size(); s++) {

            std::vector<Vertex> vertices;
            std::vector<GLuint> indices;
            std::vector<Texture> textures;
            bool isRockMaterial = false;
            bool isWindMaterial = false;
			bool isGrass = false;
			bool isFern = false;

            size_t index_offset = 0;

            // Vertex deduplication
            std::unordered_map<Vertex, GLuint, VertexHash> uniqueVertices;
            std::vector<Vertex> uniqueVerts;
            std::vector<GLuint> uniqueIndices;

            for (size_t f = 0; f < shapes[s].mesh.num_face_vertices.size(); f++) {

                int fv = shapes[s].mesh.num_face_vertices[f];
                std::vector<glm::vec3> faceVertices;
                std::vector<glm::vec2> faceTexCoords;

                for (size_t v = 0; v < fv; v++) {

                    tinyobj::index_t idx = shapes[s].mesh.indices[index_offset + v];

                    float vx = attrib.vertices[3 * idx.vertex_index + 0];
                    float vy = attrib.vertices[3 * idx.vertex_index + 1];
                    float vz = attrib.vertices[3 * idx.vertex_index + 2];
                    float nx = attrib.normals[3 * idx.normal_index + 0];
                    float ny = attrib.normals[3 * idx.normal_index + 1];
                    float nz = attrib.normals[3 * idx.normal_index + 2];
                    float tx = 0.0f;
                    float ty = 0.0f;

                    if (idx.texcoord_index != -1) {
                        tx = attrib.texcoords[2 * idx.texcoord_index + 0];
                        ty = attrib.texcoords[2 * idx.texcoord_index + 1];
                    }

                    glm::vec3 vertexPosition(vx, vy, vz);
                    glm::vec3 vertexNormal(nx, ny, nz);
                    glm::vec2 vertexTexCoords(tx, ty);

                    Vertex currentVertex;
                    currentVertex.Position = vertexPosition;
                    currentVertex.Normal = vertexNormal;
                    currentVertex.TexCoords = vertexTexCoords;

                    faceVertices.push_back(vertexPosition);
                    faceTexCoords.push_back(vertexTexCoords);

                    // Check if vertex is unique
                    if (uniqueVertices.find(currentVertex) == uniqueVertices.end()) {
                        uniqueVertices[currentVertex] = static_cast<GLuint>(uniqueVerts.size());
                        uniqueVerts.push_back(currentVertex);
                    }
                    uniqueIndices.push_back(uniqueVertices[currentVertex]);

                }

                if (fv == 3) {
                    glm::vec3 edge1 = faceVertices[1] - faceVertices[0];
                    glm::vec3 edge2 = faceVertices[2] - faceVertices[0];

                    glm::vec2 deltaUV1 = faceTexCoords[1] - faceTexCoords[0];
                    glm::vec2 deltaUV2 = faceTexCoords[2] - faceTexCoords[0];

                    float denominator = (deltaUV1.x * deltaUV2.y - deltaUV2.x * deltaUV1.y);
                    if (denominator == 0.0f) denominator = 1.0f; // Prevent division by zero

                    float f1 = 1.0f / denominator;

                    glm::vec3 tangent;
                    glm::vec3 bitangent;

                    tangent.x = f1 * (deltaUV2.y * edge1.x - deltaUV1.y * edge2.x);
                    tangent.y = f1 * (deltaUV2.y * edge1.y - deltaUV1.y * edge2.y);
                    tangent.z = f1 * (deltaUV2.y * edge1.z - deltaUV1.y * edge2.z);

                    bitangent.x = f1 * (-deltaUV2.x * edge1.x + deltaUV1.x * edge2.x);
                    bitangent.y = f1 * (-deltaUV2.x * edge1.y + deltaUV1.x * edge2.y);
                    bitangent.z = f1 * (-deltaUV2.x * edge1.z + deltaUV1.x * edge2.z);

                    tangent = glm::normalize(tangent);
                    bitangent = glm::normalize(bitangent);

                    for (size_t v = 0; v < fv; v++) {
                        uniqueVerts[uniqueIndices[uniqueIndices.size() - fv + v]].Tangent = tangent;
                        uniqueVerts[uniqueIndices[uniqueIndices.size() - fv + v]].Bitangent = bitangent;
                    }

                }

                index_offset += fv;
            }

            // Normalize Tangents and Bitangents
            for (auto& vertex : uniqueVerts) {
                vertex.Tangent = glm::normalize(vertex.Tangent);
                vertex.Bitangent = glm::normalize(vertex.Bitangent);
            }

            size_t a = shapes[s].mesh.material_ids.size();

            if (a > 0 && materials.size() > 0) {

                materialId = shapes[s].mesh.material_ids[0];
                if (materialId != -1) {
                    std::string materialName = materials[materialId].name;

                    if (materialName.find("Rock") != std::string::npos) {
                        isRockMaterial = true;
                    }
                    else if (materialName.find("Grass") != std::string::npos ||
                        materialName.find("Stem") != std::string::npos) {
                        isGrass = true;
                        isWindMaterial = true;
                    }
                    else if (materialName.find("Leaf") != std::string::npos)
                    {
                        isWindMaterial = true;
                    }
                    else if (materialName.find("Fern") != std::string::npos)
					{
						isWindMaterial = true;
						isFern = true;
					}
					if (materialName.find("Leaf_Autumn") != std::string::npos) {
						isWindMaterial = true;
						isGrass = true;

					}
                }

                if (materialId != -1) {

                    std::vector<Texture> textures;
                    std::string diffuseTexturePath = materials[materialId].diffuse_texname;

                    if (!diffuseTexturePath.empty()) {

                        Texture currentTexture;
                        currentTexture = LoadTexture(basePath + diffuseTexturePath, "diffuseTexture");
                        textures.push_back(currentTexture);
                    }

                    std::string specularTexturePath = materials[materialId].specular_texname;

                    if (!specularTexturePath.empty()) {

                        Texture currentTexture;
                        currentTexture = LoadTexture(basePath + specularTexturePath, "specularTexture");
                        textures.push_back(currentTexture);
                    }

                    std::string normalTexturePath = materials[materialId].bump_texname;

                    if (!normalTexturePath.empty()) {

                        Texture currentTexture;
                        currentTexture = LoadTexture(basePath + normalTexturePath, "normalTexture");
                        textures.push_back(currentTexture);
                    }

                    std::string dissolveTexturePath = materials[materialId].alpha_texname;

                    if (!dissolveTexturePath.empty()) {

                        Texture currentTexture;
                        currentTexture = LoadTexture(basePath + dissolveTexturePath, "dissolveTexture");
                        textures.push_back(currentTexture);
                    }

                    std::string displacementTexturePath = materials[materialId].displacement_texname;

                    if (!displacementTexturePath.empty()) {

                        Texture currentTexture;
                        currentTexture = LoadTexture(basePath + displacementTexturePath, "displacementTexture");
                        textures.push_back(currentTexture);
                    }

                    if (batches.find(materialId) == batches.end()) {
                        batches[materialId] = MeshBatch();
                        batches[materialId].textures = textures;
                    }

                    MeshBatch& batch = batches[materialId];

                    GLuint baseIndex = static_cast<GLuint>(batch.vertices.size());
                    batch.vertices.insert(batch.vertices.end(), uniqueVerts.begin(), uniqueVerts.end());
                    for (auto& idx : uniqueIndices) {
                        batch.indices.push_back(idx + baseIndex);
                    }
                    batch.indexCount += uniqueIndices.size();
                    batch.isRockMaterial = isRockMaterial;
                    batch.isWindMovable = isWindMaterial;
					batch.isGrass = isGrass;
					batch.isFern = isFern;
                }
            }

            if (a == 0 || materials.size() == 0 || materialId == -1) {
                int noMatId = -1;
                if (batches.find(noMatId) == batches.end()) {
                    batches[noMatId] = MeshBatch();
                }

                MeshBatch& batch = batches[noMatId];
                GLuint baseIndex = static_cast<GLuint>(batch.vertices.size());
                batch.vertices.insert(batch.vertices.end(), uniqueVerts.begin(), uniqueVerts.end());
                for (auto& idx : uniqueIndices) {
                    batch.indices.push_back(idx + baseIndex);
                }
                batch.indexCount += uniqueIndices.size();
				batch.isRockMaterial = isRockMaterial;
				batch.isWindMovable = isWindMaterial;
				batch.isGrass = isGrass;
            }
        }

        // Clear existing meshBatches before adding new ones
        meshBatches.clear();

        // Iterate through batches and split if necessary
        for (auto it = batches.begin(); it != batches.end(); ++it) {
            int matId = it->first;
            gps::MeshBatch batch = it->second;

			// Optimize mesh
			OptimizeMesh(matId, batch.vertices, batch.indices);
			batch.indexCount = static_cast<GLuint>(batch.indices.size());
            // Check if the batch is larger than MAX_BATCH_SIZE
            if (batch.vertices.size() > MAX_BATCH_SIZE) {
                // Split the batch into smaller sub-batches without deduplication
                std::vector<MeshBatch> subBatches = SplitBatch(batch);

                for (auto& subBatch : subBatches) {
                    subBatch.setupBuffers();
                    meshBatches.push_back(subBatch);
                    std::cout << "Sub-Batch with material ID " << matId << " has "
                        << subBatch.vertices.size() << " vertices and "
                        << subBatch.indices.size() << " indices" << std::endl;
                }
            }
            else {
                // Setup buffers normally
                batch.setupBuffers();
                meshBatches.push_back(batch);
                std::cout << "Batch with material ID " << matId << " has "
                    << batch.vertices.size() << " vertices and "
                    << batch.indices.size() << " indices" << std::endl;
            }

            std::cout << "Processed batch with material ID " << matId << std::endl;
        }

        std::sort(meshBatches.begin(), meshBatches.end(), compareMeshBatches);
        std::cout << "Total mesh batches after splitting and sorting: " << meshBatches.size() << std::endl;

		std::vector <MeshBatch*> batchPointers;
        for (auto& batch : meshBatches) {
			batchPointers.push_back(&batch);
        }
		bvh.build(batchPointers);

		std::cout << "BVH built" << std::endl;
    }

    Texture Model3D::LoadTexture(std::string path, std::string type) {

        for (int i = 0; i < loadedTextures.size(); i++) {

            if (loadedTextures[i].path == path) {

                return loadedTextures[i];
            }
        }

        Texture currentTexture;
        currentTexture.id = ReadTextureFromFile(path.c_str());
        currentTexture.type = std::string(type);
        currentTexture.path = path;

        loadedTextures.push_back(currentTexture);

        return currentTexture;
    }

    GLuint Model3D::ReadTextureFromFile(const char* file_name) {

        int x, y, n;
        int force_channels = 4;
        unsigned char* image_data = stbi_load(file_name, &x, &y, &n, force_channels);

        if (!image_data) {
            fprintf(stderr, "ERROR: could not load %s\n", file_name);
            return 0;
        }
        if ((x & (x - 1)) != 0 || (y & (y - 1)) != 0) {
            fprintf(
                stderr, "WARNING: texture %s is not power-of-2 dimensions\n", file_name
            );
        }

        int width_in_bytes = x * 4;
        unsigned char* top = NULL;
        unsigned char* bottom = NULL;
        unsigned char temp = 0;
        int half_height = y / 2;

        for (int row = 0; row < half_height; row++) {
            top = image_data + row * width_in_bytes;
            bottom = image_data + (y - row - 1) * width_in_bytes;

            for (int col = 0; col < width_in_bytes; col++) {
                temp = *top;
                *top = *bottom;
                *bottom = temp;
                top++;
                bottom++;
            }
        }

        GLenum internalFormat;
        if (std::string(file_name).find("Normal") != std::string::npos) {
            internalFormat = GL_RGB;
        }
        else if (std::string(file_name).find("Specular") != std::string::npos) {
            internalFormat = GL_RGB;
        }
        else if (std::string(file_name).find("Dissolve") != std::string::npos) {
            internalFormat = GL_RGBA;
        }
        else {
            internalFormat = GL_SRGB;
        }

        GLuint textureID;
        glGenTextures(1, &textureID);
        glBindTexture(GL_TEXTURE_2D, textureID);
        glTexImage2D(
            GL_TEXTURE_2D,
            0,
            internalFormat,
            x,
            y,
            0,
            GL_RGBA,
            GL_UNSIGNED_BYTE,
            image_data
        );
        glGenerateMipmap(GL_TEXTURE_2D);

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glBindTexture(GL_TEXTURE_2D, 0);

        stbi_image_free(image_data);

        return textureID;
    }

    Model3D::~Model3D() {

        for (size_t i = 0; i < loadedTextures.size(); i++) {

            glDeleteTextures(1, &loadedTextures.at(i).id);
        }

        for (size_t i = 0; i < meshBatches.size(); i++) {

            meshBatches[i].Cleanup();
        }
    }
}
