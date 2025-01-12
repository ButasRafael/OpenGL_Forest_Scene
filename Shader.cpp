#include "Shader.hpp"

namespace gps {

    std::string Shader::readShaderFile(std::string fileName) {
        std::ifstream shaderFile;
        std::string shaderString;

        shaderFile.open(fileName);

        if (!shaderFile.is_open()) {
            std::cerr << "Failed to open shader file: " << fileName << std::endl;
            return "";
        }

        std::stringstream shaderStringStream;

        shaderStringStream << shaderFile.rdbuf();

        shaderFile.close();

        shaderString = shaderStringStream.str();
        return shaderString;
    }

    void Shader::shaderCompileLog(GLuint shaderId) {

        GLint success;
        GLchar infoLog[512];

        glGetShaderiv(shaderId, GL_COMPILE_STATUS, &success);

        if (!success) {
            glGetShaderInfoLog(shaderId, 512, NULL, infoLog);
            std::cout << "Shader compilation error\n" << infoLog << std::endl;
        }
    }

    void Shader::shaderLinkLog(GLuint shaderProgramId) {

        GLint success;
        GLchar infoLog[512];

        glGetProgramiv(shaderProgramId, GL_LINK_STATUS, &success);
        if (!success) {
            glGetProgramInfoLog(shaderProgramId, 512, NULL, infoLog);
            std::cout << "Shader linking error\n" << infoLog << std::endl;
        }
    }

    void Shader::loadShader(std::string vertexShaderFileName, std::string fragmentShaderFileName, ShaderType type) {
        std::string v = readShaderFile(vertexShaderFileName);
        const GLchar* vertexShaderString = v.c_str();
        GLuint vertexShader;
        vertexShader = glCreateShader(GL_VERTEX_SHADER);
        glShaderSource(vertexShader, 1, &vertexShaderString, NULL);
        glCompileShader(vertexShader);
        shaderCompileLog(vertexShader);

        std::string f = readShaderFile(fragmentShaderFileName);
        const GLchar* fragmentShaderString = f.c_str();
        GLuint fragmentShader;
        fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
        glShaderSource(fragmentShader, 1, &fragmentShaderString, NULL);
        glCompileShader(fragmentShader);

        shaderCompileLog(fragmentShader);

        this->shaderProgram = glCreateProgram();
        glAttachShader(this->shaderProgram, vertexShader);
        glAttachShader(this->shaderProgram, fragmentShader);
        glLinkProgram(this->shaderProgram);

        glDeleteShader(vertexShader);
        glDeleteShader(fragmentShader);

        shaderLinkLog(this->shaderProgram);

        this->shaderType = type;
    }

    void Shader::loadShader(std::string vertexShaderFileName, std::string fragmentShaderFileName, std::string geometryShaderFileName, ShaderType type) {

        std::string v = readShaderFile(vertexShaderFileName);
        const GLchar* vertexShaderString = v.c_str();
        GLuint vertexShader;
        vertexShader = glCreateShader(GL_VERTEX_SHADER);
        glShaderSource(vertexShader, 1, &vertexShaderString, NULL);
        glCompileShader(vertexShader);
        shaderCompileLog(vertexShader);

        std::string f = readShaderFile(fragmentShaderFileName);
        const GLchar* fragmentShaderString = f.c_str();
        GLuint fragmentShader;
        fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
        glShaderSource(fragmentShader, 1, &fragmentShaderString, NULL);
        glCompileShader(fragmentShader);
        shaderCompileLog(fragmentShader);

        GLuint geometryShader = 0;
        bool hasGeometryShader = !geometryShaderFileName.empty();

        if (hasGeometryShader) {
            std::string g = readShaderFile(geometryShaderFileName);
            const GLchar* geometryShaderString = g.c_str();
            geometryShader = glCreateShader(GL_GEOMETRY_SHADER);
            glShaderSource(geometryShader, 1, &geometryShaderString, NULL);
            glCompileShader(geometryShader);
            shaderCompileLog(geometryShader);
        }

        this->shaderProgram = glCreateProgram();
        glAttachShader(this->shaderProgram, vertexShader);
        glAttachShader(this->shaderProgram, fragmentShader);

        if (hasGeometryShader) {
            glAttachShader(this->shaderProgram, geometryShader);
        }

        glLinkProgram(this->shaderProgram);

        glDeleteShader(vertexShader);
        glDeleteShader(fragmentShader);
        if (hasGeometryShader) {
            glDeleteShader(geometryShader);
        }

        shaderLinkLog(this->shaderProgram);
        this->shaderType = type;
    }

    void Shader::useShaderProgram() {
        glUseProgram(this->shaderProgram);
    }

    GLint Shader::getUniformLocation(const std::string& uniformName) {
        // 1) Check if we already have it
        if (uniformLocationCache.find(uniformName) != uniformLocationCache.end()) {
            return uniformLocationCache[uniformName];
        }

        // 2) If not, query from OpenGL
        GLint location = glGetUniformLocation(shaderProgram, uniformName.c_str());
        // 3) Store it in our map
        uniformLocationCache[uniformName] = location;

        if (location == -1) {
            std::cerr << "Warning: Uniform '" << uniformName
                << "' doesn't exist or isn't used in the shader!\n";
        }
        return location;
    }

}
