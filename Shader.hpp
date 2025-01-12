#ifndef Shader_hpp
#define Shader_hpp

#if defined (__APPLE__)
#define GL_SILENCE_DEPRECATION
#include <OpenGL/gl3.h>
#else
#define GLEW_STATIC
#include "GL/glew.h"
#endif

#include <fstream>
#include <sstream>
#include <iostream>
#include <string>
#include <unordered_map>

namespace gps {

    enum ShaderType {
        MAIN_SHADER,
        SHADOW_SHADER,
		SKYBOX_SHADER,
		WATER_SHADER,
		RAIN_SHADER,
		FIRE_SHADER,
		HDR_SHADER,
		BLUR_SHADER,
    };

    class Shader {

    public:
        GLuint shaderProgram;
        ShaderType shaderType;
        void loadShader(std::string vertexShaderFileName, std::string fragmentShaderFileName, ShaderType type);
        void loadShader(std::string vertexShaderFileName, std::string fragmentShaderFileName, std::string geometryShaderFileName, ShaderType type);

        void useShaderProgram();
        GLint getUniformLocation(const std::string& uniformName);


    private:
        std::string readShaderFile(std::string fileName);
        void shaderCompileLog(GLuint shaderId);
        void shaderLinkLog(GLuint shaderProgramId);
        std::unordered_map<std::string, GLint> uniformLocationCache;
    };

}

#endif
