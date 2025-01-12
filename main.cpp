#if defined (__APPLE__)
#define GLFW_INCLUDE_GLCOREARB
#define GL_SILENCE_DEPRECATION
#else
#define GLEW_STATIC
#include "GL/glew.h"
#endif

#include "GLFW/glfw3.h"

#include "glm/glm.hpp"
#include "glm/gtc/matrix_transform.hpp"
#include "glm/gtc/matrix_inverse.hpp"
#include "glm/gtc/type_ptr.hpp"
#include "glm/gtc/random.hpp"
#include "glm/gtc/noise.hpp"
#include "glm/gtx/string_cast.hpp"

#include "Window.h"
#include "Shader.hpp"
#include "Camera.hpp"
#include "Model3D.hpp"
#include "Skybox.hpp"
#include "WaterTile.hpp"
#include "WaterRenderer.hpp"
#include "WaterFrameBuffers.hpp"

#include "AudioManager.h"

#include <iostream>
#include <cmath>
#include <ctime>
#include <cstdlib>

// window
gps::Window myWindow;

// matrices
glm::mat4 model;
glm::mat4 view;
glm::mat4 projection;
glm::mat3 normalMatrix;

// camera
gps::Camera myCamera(
    glm::vec3(0.0f, 0.0f, 3.0f),
    glm::vec3(0.0f, 0.0f, -10.0f),
    glm::vec3(0.0f, 1.0f, 0.0f));


// models
gps::Model3D forest;
Skybox* daySkybox;
Skybox* nightSkybox;

// shaders
gps::Shader myBasicShader;
gps::Shader skyboxShader;
gps::Shader shadowShader;
gps::Shader pointShadowShader;
gps::Shader headShadowShader;
gps::Shader rainShader;
gps::Shader hdrShader;
gps::Shader fireShader;
gps::Shader blurShader;

//view modes
bool isWireframe = false;
bool isPointMode = false;
GLenum currentPolygonMode;
GLint polygonModeParams[2];

//structs
struct DirectionalLightUniforms {
    GLint direction;
    GLint ambient;
    GLint diffuse;
    GLint specular;
    GLint color;
    GLint enabled;
};

struct FlashLightUniforms {
    GLint direction;
    GLint ambient;
    GLint diffuse;
    GLint specular;
    GLint color;
    GLint enabled;
};

struct PointLightUniforms {
    GLint position;
    GLint ambient;
    GLint diffuse;
    GLint specular;
    GLint color;
    GLint constant;
    GLint linear;
    GLint quadratic;
    GLint enabled;
};

struct SpotLightUniforms {
    GLint position;
    GLint direction;
    GLint ambient;
    GLint diffuse;
    GLint specular;
    GLint color;
    GLint cutOff;
    GLint outerCutOff;
    GLint enabled;
};

struct HeadlightUniforms {
    GLint position;
    GLint direction;
    GLint ambient;
    GLint diffuse;
    GLint specular;
    GLint color;
    GLint cutOff;
    GLint outerCutOff;
    GLint enabled;
};

struct BasicShaderUniforms {
    GLint view;
    GLint projection;
    GLint viewPosition;
    GLint model;
    GLint clipPlane;
    GLint u_Time;
    GLint u_WindDirection;
    GLint u_WindStrength;
    GLint u_GustSize;
	GLint u_GustSpeed;
    GLint u_WindWaveLength;
    GLint useNormalMapping;
    GLint rainEnabled;
    GLint windEnabled;
    GLint fogEnabled;
    GLint fogColor;
    GLint gFogEnd;
	GLint gLayeredFogTop;
    GLint gFogTime;
	GLint gExpFogDensity;
    GLint globalLightIntensity;

    // Lights
    DirectionalLightUniforms dirLight;
    FlashLightUniforms flashLight;
    PointLightUniforms pointLight;
    SpotLightUniforms spotLight;
    HeadlightUniforms leftHeadlight;
    HeadlightUniforms rightHeadlight;

    //Shadows
	GLint lightSpaceMatrix;
	GLint leftHeadlightLightSpaceMatrix;
	GLint rightHeadlightLightSpaceMatrix;
    GLint shadowMap;
	GLint pointLightShadowMap;
	GLint leftHeadlightShadowMap;
	GLint rightHeadlightShadowMap;
    GLint farPlane;
};

struct RainShaderUniforms {
    GLint view;
    GLint projection;
    GLint viewPosition;

    DirectionalLightUniforms dirLight;
    FlashLightUniforms flashLight;
    PointLightUniforms pointLight;
    SpotLightUniforms spotLight;
    HeadlightUniforms leftHeadlight;
    HeadlightUniforms rightHeadlight;

    GLint environmentMap;

    GLint shininess;
    GLint maxDistance;
    GLint motionBlurIntensity;
};

struct FireShaderUniforms {
    GLint view;
    GLint projection;
    GLint viewPosition;
    GLint cameraRight;
    GLint cameraUp;
    GLint time;
    GLint flameAspectX;
    GLint flameAspectY;
    GLint fireTextureArray;
};

struct ShadowShaderUniforms {
    GLint model;
    GLint lightSpaceMatrix;
    GLint u_Time;
    GLint u_WindDirection;
    GLint u_WindStrength;
    GLint u_GustSize;
    GLint u_GustSpeed;
    GLint u_WindWaveLength;
    GLint windEnabled;
};

struct PointShadowShaderUniforms {
    GLint model;
    GLint shadowMatrices[6];
    GLint far_plane;
    GLint lightPos;
    GLint u_Time;
    GLint u_WindDirection;
    GLint u_WindStrength;
    GLint u_GustSize;
    GLint u_GustSpeed;
    GLint u_WindWaveLength;
    GLint windEnabled;
};

struct HeadShadowShaderUniforms {
    GLint model;
    GLint lightSpaceMatrixHead;
    GLint u_Time;
    GLint u_WindDirection;
    GLint u_WindStrength;
    GLint u_GustSize;
    GLint u_GustSpeed;
    GLint u_WindWaveLength;
    GLint windEnabled;
};

struct HDRShaderUniforms {
    GLint hdrBuffer;
    GLint bloomBlur;
    GLint bloom;
    GLint exposure;
};

struct BlurShaderUniforms {
    GLint horizontal;
};

struct SkyboxShaderUniforms {
    GLint view;
    GLint projection;
    GLint viewPosition;
    GLint daySkybox;
    GLint nightSkybox;
    GLint blendFactor;
};

struct AppState {
    gps::Window* window;
    AudioManager* audioManager;

};


struct DirLight {
    glm::vec3 direction;
    glm::vec3 ambient;
    glm::vec3 diffuse;
    glm::vec3 specular;
    glm::vec3 color;
    bool enabled;
};

struct PointLight {
    glm::vec3 position;
    glm::vec3 ambient;
    glm::vec3 diffuse;
    glm::vec3 specular;
    glm::vec3 color;
    float constant;
    float linear;
    float quadratic;
    bool enabled;
    float flickerPhase = 0.0f;
};


struct SpotLight {
    glm::vec3 position;
    glm::vec3 direction;
    float cutOff;
    float outerCutOff;
    glm::vec3 ambient;
    glm::vec3 diffuse;
    glm::vec3 specular;
    glm::vec3 color;
    bool enabled;
};

struct FireParticle {
    glm::vec3 position;
    glm::vec3 velocity;
    glm::vec4 color;
    float life;
    float size;
    float initialLife;
    float rotation;
    float rotationSpeed;
    int type;
    int textureIndex;
};

struct IntensityPulse {
    float duration;
    float timer;
    float intensity;
};

struct Location {
    glm::vec3 position;
    glm::vec3 target;
    glm::vec3 up;
};

//instances
RainShaderUniforms rainUniforms;
BasicShaderUniforms basicUniforms;

FireShaderUniforms fireUniforms;
ShadowShaderUniforms shadowUniforms;
PointShadowShaderUniforms pointShadowUniforms;
HeadShadowShaderUniforms headShadowUniforms;
HDRShaderUniforms hdrUniforms;
BlurShaderUniforms blurUniforms;
SkyboxShaderUniforms skyboxUniforms;

DirLight dirLight = {
    glm::vec3(0.0f, -1.0f, 0.1f),
    glm::vec3(0.02f, 0.02f, 0.05f),
    glm::vec3(0.2f, 0.2f, 0.3f),
    glm::vec3(0.1f, 0.1f, 0.15f),
     glm::vec3(0.7f, 0.7f, 1.0f),
    true
};

DirLight flashLight = {
    glm::vec3(0.0f, 0.0f, -1.0f),
    glm::vec3(0.2f, 0.2f, 0.3f),
    glm::vec3(0.8f, 0.8f, 1.0f),
    glm::vec3(1.0f, 1.0f, 1.0f),
    glm::vec3(0.5f, 0.5f, 1.0f),
    false
};

PointLight pointLight = {
    glm::vec3(0.0f, 0.3f, -7.3f),    
    glm::vec3(0.15f, 0.07f, 0.02f),   
    glm::vec3(1.0f, 0.55f, 0.2f),    
    glm::vec3(1.0f, 0.9f, 0.5f),      
    glm::vec3(1.0f, 0.7f, 0.3f),    
    1.0f,                            
    0.09f,                        
    0.032f,                          
    true
};

SpotLight spotLight = {
    glm::vec3(myCamera.getPosition()),
    glm::vec3(myCamera.getFront()),
    glm::cos(glm::radians(12.5f)),
    glm::cos(glm::radians(17.5f)),
    glm::vec3(0.02f, 0.02f, 0.02f),   
    glm::vec3(0.9f, 0.9f, 1.0f),     
    glm::vec3(1.0f, 1.0f, 1.0f),      
    glm::vec3(1.0f, 1.0f, 1.0f), 
    true
};

SpotLight leftHeadlight = {
    glm::vec3(0.6f, 1.4f, 4.0f),
    glm::vec3(0.0f, -0.2f, 1.0f),
    glm::cos(glm::radians(10.0f)),
    glm::cos(glm::radians(15.0f)),   
    glm::vec3(0.02f, 0.02f, 0.02f), 
    glm::vec3(2.0f, 2.0f, 1.9f),    
    glm::vec3(2.5f, 2.5f, 2.4f),
    glm::vec3(1.0f, 0.95f, 0.9f),  
    true
};

SpotLight rightHeadlight = {
    glm::vec3(2.0f, 1.4f, 4.0f),
    glm::vec3(0.0f, -0.2f, 1.0f),
    glm::cos(glm::radians(12.5f)),
    glm::cos(glm::radians(17.5f)),
    glm::vec3(0.02f, 0.02f, 0.02f),
    glm::vec3(2.0f, 2.0f, 1.9f),
    glm::vec3(2.5f, 2.5f, 2.4f),
    glm::vec3(1.0f, 0.95f, 0.9f),
    true
};

std::vector<FireParticle> fireParticles;
std::vector<IntensityPulse> activePulses;
AudioManager audioManager;

GLfloat cameraSpeed = 0.5f;
GLfloat angle;
GLboolean pressedKeys[1024];

//Fog
bool fogEnabled = false;
glm::vec3 dayFogColor = glm::vec3(0.8f, 0.8f, 0.9f);
glm::vec3 nightFogColor = glm::vec3(0.3f, 0.3f, 0.4f);
float daygFogEnd = 50.0f;
float nightgFogEnd = 30.0f;
float daygLayeredFogTop = 15.0f;
float nightgLayeredFogTop = 15.0f;
float daygExpFogDensity = 0.02;
float nightgExpFogDensity = 0.04;
float fogTime = 0.0f;

//Day-Night mode
bool isDay = true;
bool transitioning = false;
glm::vec3 dayAmbient = glm::vec3(0.3f, 0.3f, 0.3f);
glm::vec3 dayDiffuse = glm::vec3(1.0f, 1.0f, 0.9f);
glm::vec3 daySpecular = glm::vec3(0.5f, 0.5f, 0.5f);
glm::vec3 dayColor = glm::vec3(1.0f, 0.95f, 0.8f);
glm::vec3 lakeDayPosition = glm::vec3(45.0f, 100.0f, -50.0f);
glm::vec3 lakeDayColor = glm::vec3(1.0f, 1.0f, 1.0f);


glm::vec3 nightAmbient = glm::vec3(0.02f, 0.02f, 0.05f);
glm::vec3 nightDiffuse = glm::vec3(0.2f, 0.2f, 0.3f);
glm::vec3 nightSpecular = glm::vec3(0.1f, 0.1f, 0.15f);
glm::vec3 nightColor = glm::vec3(0.7f, 0.7f, 1.0f);

glm::vec3 lakeNightPosition = glm::vec3(20.0f, 30.0f, 40.0f);
glm::vec3 lakeNightColor = glm::vec3(0.0f, 0.2f, 0.4f);


glm::vec3 dayDirection = glm::vec3(-0.3f, -1.0f, 0.1f);
glm::vec3 nightDirection = glm::vec3(0.0f, -1.0f, 0.1f);


//normal mapping
bool useNormalMapping = true;


//tour
std::vector<Location> keyLocations;
bool isTourActive = false;
size_t currentWaypoint = 0;
float t = 0.0f;
const float tIncrement = 0.005f;
float blend = 1.0f;
float transitionDuration = 3.0f;
float transitionStartTime = 0.0f;
float startBlend = 1.0f;
float targetBlend = 1.0f;

//waypoints recording
bool recordingKeyPressed = false;
bool saveKeyPressed = false;
bool loadKeyPressed = false;
bool removeKeyPressed = false;


//rain
#define GRAVITY -4.905f
#define TERMINAL_VELOCITY -25.0f
GLuint rainVAO, rainVBO, rainParamsVBO;
std::vector<glm::vec2> raindropParams;
std::vector<glm::vec3> raindrops, raindropsVelocity;
const int NUM_RAINDROPS = 1000000;
bool rainEnabled = false;
bool rainPlaying = false;
float rainMinX = -100.0f;
float rainMaxX = 100.0f;
float rainMinZ = -100.0f;
float rainMaxZ = 100.0f;
float rainTopY = 25.0f;
float rainBottomY = -1.0f;

//fire
const GLfloat quad2Vertices[] = {
    -0.5f, -0.5f,   0.0f, 0.0f,
     0.5f, -0.5f,   1.0f, 0.0f,
     0.5f,  0.5f,   1.0f, 1.0f,

    -0.5f, -0.5f,   0.0f, 0.0f,
     0.5f,  0.5f,   1.0f, 1.0f,
    -0.5f,  0.5f,   0.0f, 1.0f
};
GLuint fireTextureArray;
GLuint quad2VAO, quad2VBO, instanceVBO;
bool firePlaying = false;
const float FLICKER_BASE_FREQUENCY = 5.0f;
const float FLICKER_AMPLITUDE_AMBIENT = 0.05f;
const float FLICKER_AMPLITUDE_DIFFUSE = 0.1f;
const float FLICKER_AMPLITUDE_SPECULAR = 0.15f;
const float FLICKER_RANDOMNESS = 0.02f;
const int MAX_FIRE_PARTICLES = 10000;
const int MAX_SMOKE_PARTICLES = 4000;
const int MAX_EMBER_PARTICLES = 1000;
const int MAX_TOTAL_PARTICLES = MAX_FIRE_PARTICLES + MAX_SMOKE_PARTICLES + MAX_EMBER_PARTICLES;
float spawnRadius = 0.2f;
float minUpVelocity = 1.8f;
float maxUpVelocity = 2.6f;
float minHorizontalVel = -0.15f;
float maxHorizontalVel = 0.15f;

//wind
bool windEnabled = false;
float u_Time = 0.0f;
glm::vec3 u_WindDirection = glm::normalize(glm::vec3(1.0f, 0.0f, 0.0f));
float u_WindStrength = 0.75f; 
float gustSize = 0.75f;
float gustSpeed = 0.5f;
float windWaveLength = 2.0f;


//lightning
bool isFlashing = false;
float flashDuration = 0.2f;
float flashTimer = 0.0f;
float globalLightIntensity = 1.0f;
float flashIntervalMin = 7.0f;
float flashIntervalMax = 14.0f;
float nextFlashTime = 0.0f;
float pulseIntervalMin = 0.05f;
float pulseIntervalMax = 0.3f;
float timeSinceLastPulse = 0.0f;

//shadows
GLuint shadowMapFBO, depthMap;
GLuint pointLightFBO, depthCubemap;
GLuint leftHeadlightFBO, leftHeadlightDepthMap;
GLuint rightHeadlightFBO, rightHeadlightDepthMap;
const GLuint SHADOW_WIDTH = 4096, SHADOW_HEIGHT = 4096;
const GLuint SPOT_LIGHT_SHADOW_WIDTH = 1024, SPOT_LIGHT_SHADOW_HEIGHT = 1024;
const GLuint POINT_SHADOW_WIDTH = 1024, POINT_SHADOW_HEIGHT = 1024;
glm::mat4 lightProjection, lightView;
glm::mat4 lightSpaceMatrix;

//hdr
GLuint hdrFBO, colorBuffer, rbo;
GLuint quadVAO = 0;
GLuint quadVBO;
bool hdrEnabled = true;
bool performHDR = true;
float exposure = 1.0f;

//bloom
GLuint colorBuffers[2];
GLuint pingpongFBO[2];
GLuint pingpongColorbuffers[2];
bool bloomEnabled = false;
bool bloomKeyPressed = false;
unsigned int blurIterations = 10;

//lake
WaterRenderer* waterRenderer;
std::vector<WaterTile> waterTiles;
WaterFrameBuffers* waterFrameBuffers;
glm::vec3 lakeLightPosition = glm::vec3(0.0f, 0.0f, 0.0f);
glm::vec3 lakeLightColor = glm::vec3(0.0f, 0.0f, 0.0f);


//error checker
GLenum glCheckError_(const char* file, int line)
{
    GLenum errorCode;
    while ((errorCode = glGetError()) != GL_NO_ERROR) {
        std::string error;
        switch (errorCode) {
        case GL_INVALID_ENUM:
            error = "INVALID_ENUM";
            break;
        case GL_INVALID_VALUE:
            error = "INVALID_VALUE";
            break;
        case GL_INVALID_OPERATION:
            error = "INVALID_OPERATION";
            break;
        case GL_OUT_OF_MEMORY:
            error = "OUT_OF_MEMORY";
            break;
        case GL_INVALID_FRAMEBUFFER_OPERATION:
            error = "INVALID_FRAMEBUFFER_OPERATION";
            break;
        }
        std::cout << error << " | " << file << " (" << line << ")" << std::endl;
    }
    return errorCode;
}
#define glCheckError() glCheckError_(__FILE__, __LINE__)


//texture array loader
GLuint LoadFireTextureArray(const std::vector<std::string>& filePaths)
{
    if (filePaths.empty()) {
        std::cerr << "No texture paths provided." << std::endl;
        return 0;
    }

    int width, height, numChannels;

    unsigned char* data = stbi_load(filePaths[0].c_str(), &width, &height, &numChannels, 4);
    if (!data) {
        std::cerr << "Failed to load texture: " << filePaths[0] << std::endl;
        return 0;
    }
    stbi_image_free(data);

    GLuint texArrayID;
    glGenTextures(1, &texArrayID);
    glBindTexture(GL_TEXTURE_2D_ARRAY, texArrayID);

    glTexImage3D(
        GL_TEXTURE_2D_ARRAY,
        0,
        GL_RGBA8,
        width,
        height,
        static_cast<GLsizei>(filePaths.size()),
        0,
        GL_RGBA,
        GL_UNSIGNED_BYTE,
        nullptr
    );

    for (size_t i = 0; i < filePaths.size(); i++)
    {
        int w, h, ch;
        unsigned char* layerData = stbi_load(filePaths[i].c_str(), &w, &h, &ch, 4);
        if (!layerData) {
            std::cerr << "Failed to load texture: " << filePaths[i] << std::endl;
            continue;
        }

        if (w != width || h != height) {
            std::cerr << "Texture size mismatch in " << filePaths[i]
                << ". Expected (" << width << "x" << height
                << "), got (" << w << "x" << h << ")." << std::endl;
            stbi_image_free(layerData);
            continue;
        }

        glTexSubImage3D(
            GL_TEXTURE_2D_ARRAY,
            0,
            0, 0, static_cast<GLint>(i),
            w, h, 1,
            GL_RGBA,
            GL_UNSIGNED_BYTE,
            layerData
        );

        stbi_image_free(layerData);
    }

    glGenerateMipmap(GL_TEXTURE_2D_ARRAY);

    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    glBindTexture(GL_TEXTURE_2D_ARRAY, 0);

    return texArrayID;
}

//tour interpolation and waypoints generation
glm::vec3 catmullRomInterpolate(const glm::vec3& P0, const glm::vec3& P1,
    const glm::vec3& P2, const glm::vec3& P3, float t) {
    return 0.5f * (
        (2.0f * P1) +
        (-P0 + P2) * t +
        (2.0f * P0 - 5.0f * P1 + 4.0f * P2 - P3) * t * t +
        (-P0 + 3.0f * P1 - 3.0f * P2 + P3) * t * t * t
        );
}

float easeInOut(float t) {
    return t < 0.5f ? 2.0f * t * t : -1.0f + (4.0f - 2.0f * t) * t;
}
void recordWaypoint() {

    glm::vec3 currentPos = myCamera.getPosition();
    glm::vec3 currentTarget = myCamera.getTarget();
    glm::vec3 currentUp = myCamera.getUpDirection();

    Location newLocation;
    newLocation.position = currentPos;
    newLocation.target = currentTarget;
    newLocation.up = currentUp;

    keyLocations.push_back(newLocation);

    std::cout << "Waypoint Recorded: " << keyLocations.size() - 1 << std::endl;
    std::cout << "Position: " << glm::to_string(currentPos) << std::endl;
    std::cout << "Target: " << glm::to_string(currentTarget) << std::endl;
    std::cout << "Up: " << glm::to_string(currentUp) << std::endl;

}

bool saveWaypoints(const std::string& filename) {
    std::ofstream outFile(filename);
    if (!outFile.is_open()) {
        std::cerr << "Error: Could not open " << filename << " for writing." << std::endl;
        return false;
    }

    for (const auto& location : keyLocations) {
        outFile << location.position.x << " " << location.position.y << " " << location.position.z << " "
            << location.target.x << " " << location.target.y << " " << location.target.z << " "
            << location.up.x << " " << location.up.y << " " << location.up.z << "\n";
    }

    outFile.close();
    std::cout << "Waypoints successfully saved to " << filename << std::endl;
    return true;
}

bool loadWaypoints(const std::string& filename) {
    std::ifstream inFile(filename);
    if (!inFile.is_open()) {
        std::cerr << "Warning: Could not open " << filename << " for reading. Starting with default waypoints." << std::endl;
        return false;
    }

    keyLocations.clear();

    std::string line;
    while (std::getline(inFile, line)) {
        std::istringstream iss(line);
        Location loc;
        if (!(iss >> loc.position.x >> loc.position.y >> loc.position.z
            >> loc.target.x >> loc.target.y >> loc.target.z
            >> loc.up.x >> loc.up.y >> loc.up.z)) {
            std::cerr << "Error: Malformed line in " << filename << ": " << line << std::endl;
            continue;
        }
        keyLocations.push_back(loc);
    }

    inFile.close();
    std::cout << "Waypoints successfully loaded from " << filename << ". Total Waypoints: " << keyLocations.size() << std::endl;
    return true;
}


//retrievers
void retrieveRainUniformLocations() {
    rainShader.useShaderProgram();

    rainUniforms.view = glGetUniformLocation(rainShader.shaderProgram, "view");
    rainUniforms.projection = glGetUniformLocation(rainShader.shaderProgram, "projection");
    rainUniforms.viewPosition = glGetUniformLocation(rainShader.shaderProgram, "viewPosition");

    rainUniforms.dirLight.direction = glGetUniformLocation(rainShader.shaderProgram, "dirLight.direction");
    rainUniforms.dirLight.ambient = glGetUniformLocation(rainShader.shaderProgram, "dirLight.ambient");
    rainUniforms.dirLight.diffuse = glGetUniformLocation(rainShader.shaderProgram, "dirLight.diffuse");
    rainUniforms.dirLight.specular = glGetUniformLocation(rainShader.shaderProgram, "dirLight.specular");
    rainUniforms.dirLight.color = glGetUniformLocation(rainShader.shaderProgram, "dirLight.color");
    rainUniforms.dirLight.enabled = glGetUniformLocation(rainShader.shaderProgram, "dirLight.enabled");

	rainUniforms.flashLight.direction = glGetUniformLocation(rainShader.shaderProgram, "flashLight.direction");
	rainUniforms.flashLight.ambient = glGetUniformLocation(rainShader.shaderProgram, "flashLight.ambient");
	rainUniforms.flashLight.diffuse = glGetUniformLocation(rainShader.shaderProgram, "flashLight.diffuse");
	rainUniforms.flashLight.specular = glGetUniformLocation(rainShader.shaderProgram, "flashLight.specular");
	rainUniforms.flashLight.color = glGetUniformLocation(rainShader.shaderProgram, "flashLight.color");
	rainUniforms.flashLight.enabled = glGetUniformLocation(rainShader.shaderProgram, "flashLight.enabled");

	rainUniforms.pointLight.position = glGetUniformLocation(rainShader.shaderProgram, "pointLight.position");
	rainUniforms.pointLight.ambient = glGetUniformLocation(rainShader.shaderProgram, "pointLight.ambient");
	rainUniforms.pointLight.diffuse = glGetUniformLocation(rainShader.shaderProgram, "pointLight.diffuse");
	rainUniforms.pointLight.specular = glGetUniformLocation(rainShader.shaderProgram, "pointLight.specular");
	rainUniforms.pointLight.color = glGetUniformLocation(rainShader.shaderProgram, "pointLight.color");
	rainUniforms.pointLight.constant = glGetUniformLocation(rainShader.shaderProgram, "pointLight.constant");
	rainUniforms.pointLight.linear = glGetUniformLocation(rainShader.shaderProgram, "pointLight.linear");
	rainUniforms.pointLight.quadratic = glGetUniformLocation(rainShader.shaderProgram, "pointLight.quadratic");
	rainUniforms.pointLight.enabled = glGetUniformLocation(rainShader.shaderProgram, "pointLight.enabled");

	rainUniforms.spotLight.position = glGetUniformLocation(rainShader.shaderProgram, "spotLight.position");
	rainUniforms.spotLight.direction = glGetUniformLocation(rainShader.shaderProgram, "spotLight.direction");
	rainUniforms.spotLight.ambient = glGetUniformLocation(rainShader.shaderProgram, "spotLight.ambient");
	rainUniforms.spotLight.diffuse = glGetUniformLocation(rainShader.shaderProgram, "spotLight.diffuse");
	rainUniforms.spotLight.specular = glGetUniformLocation(rainShader.shaderProgram, "spotLight.specular");
	rainUniforms.spotLight.color = glGetUniformLocation(rainShader.shaderProgram, "spotLight.color");
	rainUniforms.spotLight.cutOff = glGetUniformLocation(rainShader.shaderProgram, "spotLight.cutOff");
	rainUniforms.spotLight.outerCutOff = glGetUniformLocation(rainShader.shaderProgram, "spotLight.outerCutOff");
	rainUniforms.spotLight.enabled = glGetUniformLocation(rainShader.shaderProgram, "spotLight.enabled");

	rainUniforms.leftHeadlight.position = glGetUniformLocation(rainShader.shaderProgram, "leftHeadlight.position");
	rainUniforms.leftHeadlight.direction = glGetUniformLocation(rainShader.shaderProgram, "leftHeadlight.direction");
	rainUniforms.leftHeadlight.ambient = glGetUniformLocation(rainShader.shaderProgram, "leftHeadlight.ambient");
	rainUniforms.leftHeadlight.diffuse = glGetUniformLocation(rainShader.shaderProgram, "leftHeadlight.diffuse");
	rainUniforms.leftHeadlight.specular = glGetUniformLocation(rainShader.shaderProgram, "leftHeadlight.specular");
	rainUniforms.leftHeadlight.color = glGetUniformLocation(rainShader.shaderProgram, "leftHeadlight.color");
	rainUniforms.leftHeadlight.cutOff = glGetUniformLocation(rainShader.shaderProgram, "leftHeadlight.cutOff");
	rainUniforms.leftHeadlight.outerCutOff = glGetUniformLocation(rainShader.shaderProgram, "leftHeadlight.outerCutOff");
	rainUniforms.leftHeadlight.enabled = glGetUniformLocation(rainShader.shaderProgram, "leftHeadlight.enabled");

	rainUniforms.rightHeadlight.position = glGetUniformLocation(rainShader.shaderProgram, "rightHeadlight.position");
	rainUniforms.rightHeadlight.direction = glGetUniformLocation(rainShader.shaderProgram, "rightHeadlight.direction");
	rainUniforms.rightHeadlight.ambient = glGetUniformLocation(rainShader.shaderProgram, "rightHeadlight.ambient");
	rainUniforms.rightHeadlight.diffuse = glGetUniformLocation(rainShader.shaderProgram, "rightHeadlight.diffuse");
	rainUniforms.rightHeadlight.specular = glGetUniformLocation(rainShader.shaderProgram, "rightHeadlight.specular");
	rainUniforms.rightHeadlight.color = glGetUniformLocation(rainShader.shaderProgram, "rightHeadlight.color");
	rainUniforms.rightHeadlight.cutOff = glGetUniformLocation(rainShader.shaderProgram, "rightHeadlight.cutOff");
	rainUniforms.rightHeadlight.outerCutOff = glGetUniformLocation(rainShader.shaderProgram, "rightHeadlight.outerCutOff");
	rainUniforms.rightHeadlight.enabled = glGetUniformLocation(rainShader.shaderProgram, "rightHeadlight.enabled");

    rainUniforms.environmentMap = glGetUniformLocation(rainShader.shaderProgram, "environmentMap");

    rainUniforms.shininess = glGetUniformLocation(rainShader.shaderProgram, "shininess");
    rainUniforms.maxDistance = glGetUniformLocation(rainShader.shaderProgram, "maxDistance");
    rainUniforms.motionBlurIntensity = glGetUniformLocation(rainShader.shaderProgram, "motionBlurIntensity");
}

void retrieveBasicUniformLocations() {
    myBasicShader.useShaderProgram();

    basicUniforms.model = glGetUniformLocation(myBasicShader.shaderProgram, "model");
    basicUniforms.view = glGetUniformLocation(myBasicShader.shaderProgram, "view");
    basicUniforms.projection = glGetUniformLocation(myBasicShader.shaderProgram, "projection");
    basicUniforms.viewPosition = glGetUniformLocation(myBasicShader.shaderProgram, "viewPos");
    basicUniforms.clipPlane = glGetUniformLocation(myBasicShader.shaderProgram, "plane");
    basicUniforms.u_Time = glGetUniformLocation(myBasicShader.shaderProgram, "u_Time");
    basicUniforms.u_WindDirection = glGetUniformLocation(myBasicShader.shaderProgram, "u_WindDirection");
    basicUniforms.u_WindStrength = glGetUniformLocation(myBasicShader.shaderProgram, "u_WindStrength");
	basicUniforms.u_GustSize = glGetUniformLocation(myBasicShader.shaderProgram, "u_GustSize");
	basicUniforms.u_GustSpeed = glGetUniformLocation(myBasicShader.shaderProgram, "u_GustSpeed");
	basicUniforms.u_WindWaveLength = glGetUniformLocation(myBasicShader.shaderProgram, "u_WindWaveLength");

    basicUniforms.dirLight.direction = glGetUniformLocation(myBasicShader.shaderProgram, "dirLight.direction");
    basicUniforms.dirLight.ambient = glGetUniformLocation(myBasicShader.shaderProgram, "dirLight.ambient");
    basicUniforms.dirLight.diffuse = glGetUniformLocation(myBasicShader.shaderProgram, "dirLight.diffuse");
    basicUniforms.dirLight.specular = glGetUniformLocation(myBasicShader.shaderProgram, "dirLight.specular");
    basicUniforms.dirLight.color = glGetUniformLocation(myBasicShader.shaderProgram, "dirLight.color");
    basicUniforms.dirLight.enabled = glGetUniformLocation(myBasicShader.shaderProgram, "dirLight.enabled");

	basicUniforms.flashLight.direction = glGetUniformLocation(myBasicShader.shaderProgram, "flashLight.direction");
	basicUniforms.flashLight.ambient = glGetUniformLocation(myBasicShader.shaderProgram, "flashLight.ambient");
	basicUniforms.flashLight.diffuse = glGetUniformLocation(myBasicShader.shaderProgram, "flashLight.diffuse");
	basicUniforms.flashLight.specular = glGetUniformLocation(myBasicShader.shaderProgram, "flashLight.specular");
	basicUniforms.flashLight.color = glGetUniformLocation(myBasicShader.shaderProgram, "flashLight.color");
	basicUniforms.flashLight.enabled = glGetUniformLocation(myBasicShader.shaderProgram, "flashLight.enabled");

	basicUniforms.pointLight.position = glGetUniformLocation(myBasicShader.shaderProgram, "pointLight.position");
	basicUniforms.pointLight.ambient = glGetUniformLocation(myBasicShader.shaderProgram, "pointLight.ambient");
	basicUniforms.pointLight.diffuse = glGetUniformLocation(myBasicShader.shaderProgram, "pointLight.diffuse");
	basicUniforms.pointLight.specular = glGetUniformLocation(myBasicShader.shaderProgram, "pointLight.specular");
	basicUniforms.pointLight.color = glGetUniformLocation(myBasicShader.shaderProgram, "pointLight.color");
	basicUniforms.pointLight.constant = glGetUniformLocation(myBasicShader.shaderProgram, "pointLight.constant");
	basicUniforms.pointLight.linear = glGetUniformLocation(myBasicShader.shaderProgram, "pointLight.linear");
	basicUniforms.pointLight.quadratic = glGetUniformLocation(myBasicShader.shaderProgram, "pointLight.quadratic");
	basicUniforms.pointLight.enabled = glGetUniformLocation(myBasicShader.shaderProgram, "pointLight.enabled");

	basicUniforms.spotLight.position = glGetUniformLocation(myBasicShader.shaderProgram, "spotLight.position");
	basicUniforms.spotLight.direction = glGetUniformLocation(myBasicShader.shaderProgram, "spotLight.direction");
	basicUniforms.spotLight.ambient = glGetUniformLocation(myBasicShader.shaderProgram, "spotLight.ambient");
	basicUniforms.spotLight.diffuse = glGetUniformLocation(myBasicShader.shaderProgram, "spotLight.diffuse");
    basicUniforms.spotLight.specular = glGetUniformLocation(myBasicShader.shaderProgram, "spotLight.specular");
	basicUniforms.spotLight.color = glGetUniformLocation(myBasicShader.shaderProgram, "spotLight.color");
	basicUniforms.spotLight.cutOff = glGetUniformLocation(myBasicShader.shaderProgram, "spotLight.cutOff");
	basicUniforms.spotLight.outerCutOff = glGetUniformLocation(myBasicShader.shaderProgram, "spotLight.outerCutOff");
	basicUniforms.spotLight.enabled = glGetUniformLocation(myBasicShader.shaderProgram, "spotLight.enabled");

	basicUniforms.leftHeadlight.position = glGetUniformLocation(myBasicShader.shaderProgram, "leftHeadlight.position");
	basicUniforms.leftHeadlight.direction = glGetUniformLocation(myBasicShader.shaderProgram, "leftHeadlight.direction");
	basicUniforms.leftHeadlight.ambient = glGetUniformLocation(myBasicShader.shaderProgram, "leftHeadlight.ambient");
    basicUniforms.leftHeadlight.diffuse = glGetUniformLocation(myBasicShader.shaderProgram, "leftHeadlight");
	basicUniforms.leftHeadlight.specular = glGetUniformLocation(myBasicShader.shaderProgram, "leftHeadlight.specular");
	basicUniforms.leftHeadlight.color = glGetUniformLocation(myBasicShader.shaderProgram, "leftHeadlight.color");
	basicUniforms.leftHeadlight.cutOff = glGetUniformLocation(myBasicShader.shaderProgram, "leftHeadlight.cutOff");
	basicUniforms.leftHeadlight.outerCutOff = glGetUniformLocation(myBasicShader.shaderProgram, "leftHeadlight.outerCutOff");
	basicUniforms.leftHeadlight.enabled = glGetUniformLocation(myBasicShader.shaderProgram, "leftHeadlight.enabled");

	basicUniforms.rightHeadlight.position = glGetUniformLocation(myBasicShader.shaderProgram, "rightHeadlight.position");
	basicUniforms.rightHeadlight.direction = glGetUniformLocation(myBasicShader.shaderProgram, "rightHeadlight.direction");
	basicUniforms.rightHeadlight.ambient = glGetUniformLocation(myBasicShader.shaderProgram, "rightHeadlight.ambient");
	basicUniforms.rightHeadlight.diffuse = glGetUniformLocation(myBasicShader.shaderProgram, "rightHeadlight.diffuse");
	basicUniforms.rightHeadlight.specular = glGetUniformLocation(myBasicShader.shaderProgram, "rightHeadlight.specular");
	basicUniforms.rightHeadlight.color = glGetUniformLocation(myBasicShader.shaderProgram, "rightHeadlight.color");
	basicUniforms.rightHeadlight.cutOff = glGetUniformLocation(myBasicShader.shaderProgram, "rightHeadlight.cutOff");
	basicUniforms.rightHeadlight.outerCutOff = glGetUniformLocation(myBasicShader.shaderProgram, "rightHeadlight.outerCutOff");
	basicUniforms.rightHeadlight.enabled = glGetUniformLocation(myBasicShader.shaderProgram, "rightHeadlight.enabled");

    basicUniforms.useNormalMapping = glGetUniformLocation(myBasicShader.shaderProgram, "useNormalMapping");
    basicUniforms.rainEnabled = glGetUniformLocation(myBasicShader.shaderProgram, "rainEnabled");
    basicUniforms.windEnabled = glGetUniformLocation(myBasicShader.shaderProgram, "windEnabled");
    basicUniforms.fogEnabled = glGetUniformLocation(myBasicShader.shaderProgram, "fogEnabled");
    basicUniforms.fogColor = glGetUniformLocation(myBasicShader.shaderProgram, "fogColor");
	basicUniforms.gFogEnd = glGetUniformLocation(myBasicShader.shaderProgram, "gFogEnd");
	basicUniforms.gLayeredFogTop = glGetUniformLocation(myBasicShader.shaderProgram, "gLayeredFogTop");
	basicUniforms.gExpFogDensity = glGetUniformLocation(myBasicShader.shaderProgram, "gExpFogDensity");
	basicUniforms.gFogTime = glGetUniformLocation(myBasicShader.shaderProgram, "gFogTime");
    basicUniforms.globalLightIntensity = glGetUniformLocation(myBasicShader.shaderProgram, "globalLightIntensity");

	basicUniforms.lightSpaceMatrix = glGetUniformLocation(myBasicShader.shaderProgram, "lightSpaceMatrix");
	basicUniforms.leftHeadlightLightSpaceMatrix = glGetUniformLocation(myBasicShader.shaderProgram, "leftHeadlightLightSpaceMatrix");
	basicUniforms.rightHeadlightLightSpaceMatrix = glGetUniformLocation(myBasicShader.shaderProgram, "rightHeadlightLightSpaceMatrix");
	basicUniforms.shadowMap = glGetUniformLocation(myBasicShader.shaderProgram, "shadowMap");
	basicUniforms.pointLightShadowMap = glGetUniformLocation(myBasicShader.shaderProgram, "pointLightShadowMap");
	basicUniforms.leftHeadlightShadowMap = glGetUniformLocation(myBasicShader.shaderProgram, "leftHeadlightShadowMap");
	basicUniforms.rightHeadlightShadowMap = glGetUniformLocation(myBasicShader.shaderProgram, "rightHeadlightShadowMap");
	basicUniforms.farPlane = glGetUniformLocation(myBasicShader.shaderProgram, "farPlane");
}

void retrieveFireUniformLocations() {
    fireShader.useShaderProgram();

    fireUniforms.view = glGetUniformLocation(fireShader.shaderProgram, "view");
    fireUniforms.projection = glGetUniformLocation(fireShader.shaderProgram, "projection");
	fireUniforms.viewPosition = glGetUniformLocation(fireShader.shaderProgram, "viewPosition");

	fireUniforms.cameraRight = glGetUniformLocation(fireShader.shaderProgram, "cameraRight");
	fireUniforms.cameraUp = glGetUniformLocation(fireShader.shaderProgram, "cameraUp");
	fireUniforms.time = glGetUniformLocation(fireShader.shaderProgram, "time");
	fireUniforms.flameAspectX = glGetUniformLocation(fireShader.shaderProgram, "flameAspectX");
	fireUniforms.flameAspectY = glGetUniformLocation(fireShader.shaderProgram, "flameAspectY");
	fireUniforms.fireTextureArray = glGetUniformLocation(fireShader.shaderProgram, "fireTextureArray");
}

void retrieveShadowUniformLocations() {
    shadowShader.useShaderProgram();
	shadowUniforms.model = glGetUniformLocation(shadowShader.shaderProgram, "model");
	shadowUniforms.lightSpaceMatrix = glGetUniformLocation(shadowShader.shaderProgram, "lightSpaceMatrix");
	shadowUniforms.u_Time = glGetUniformLocation(shadowShader.shaderProgram, "u_Time");
	shadowUniforms.u_WindDirection = glGetUniformLocation(shadowShader.shaderProgram, "u_WindDirection");
	shadowUniforms.u_WindStrength = glGetUniformLocation(shadowShader.shaderProgram, "u_WindStrength");
	shadowUniforms.u_GustSize = glGetUniformLocation(shadowShader.shaderProgram, "u_GustSize");
	shadowUniforms.u_GustSpeed = glGetUniformLocation(shadowShader.shaderProgram, "u_GustSpeed");
	shadowUniforms.u_WindWaveLength = glGetUniformLocation(shadowShader.shaderProgram, "u_WindWaveLength");
	shadowUniforms.windEnabled = glGetUniformLocation(shadowShader.shaderProgram, "windEnabled");
}

void retrievePointShadowUniformLocations() {
    pointShadowShader.useShaderProgram();
	pointShadowUniforms.model = glGetUniformLocation(pointShadowShader.shaderProgram, "model");
    for (unsigned int i = 0; i < 6; ++i) {
        std::string uniformName = "shadowMatrices[" + std::to_string(i) + "]";
		pointShadowUniforms.shadowMatrices[i] = glGetUniformLocation(pointShadowShader.shaderProgram, uniformName.c_str());
    }
	pointShadowUniforms.far_plane = glGetUniformLocation(pointShadowShader.shaderProgram, "far_plane");
	pointShadowUniforms.lightPos = glGetUniformLocation(pointShadowShader.shaderProgram, "lightPos");
	pointShadowUniforms.u_Time = glGetUniformLocation(pointShadowShader.shaderProgram, "u_Time");
	pointShadowUniforms.u_WindDirection = glGetUniformLocation(pointShadowShader.shaderProgram, "u_WindDirection");
	pointShadowUniforms.u_WindStrength = glGetUniformLocation(pointShadowShader.shaderProgram, "u_WindStrength");
	pointShadowUniforms.u_GustSize = glGetUniformLocation(pointShadowShader.shaderProgram, "u_GustSize");
	pointShadowUniforms.u_GustSpeed = glGetUniformLocation(pointShadowShader.shaderProgram, "u_GustSpeed");
	pointShadowUniforms.u_WindWaveLength = glGetUniformLocation(pointShadowShader.shaderProgram, "u_WindWaveLength");
	pointShadowUniforms.windEnabled = glGetUniformLocation(pointShadowShader.shaderProgram, "windEnabled");
}

void retrieveHeadShadowUniformLocations() {
    headShadowShader.useShaderProgram();
	headShadowUniforms.model = glGetUniformLocation(headShadowShader.shaderProgram, "model");
	headShadowUniforms.lightSpaceMatrixHead = glGetUniformLocation(headShadowShader.shaderProgram, "lightSpaceMatrixHead");
	headShadowUniforms.u_Time = glGetUniformLocation(headShadowShader.shaderProgram, "u_Time");
	headShadowUniforms.u_WindDirection = glGetUniformLocation(headShadowShader.shaderProgram, "u_WindDirection");
	headShadowUniforms.u_WindStrength = glGetUniformLocation(headShadowShader.shaderProgram, "u_WindStrength");
	headShadowUniforms.u_GustSize = glGetUniformLocation(headShadowShader.shaderProgram, "u_GustSize");
	headShadowUniforms.u_GustSpeed = glGetUniformLocation(headShadowShader.shaderProgram, "u_GustSpeed");
	headShadowUniforms.u_WindWaveLength = glGetUniformLocation(headShadowShader.shaderProgram, "u_WindWaveLength");
	headShadowUniforms.windEnabled = glGetUniformLocation(headShadowShader.shaderProgram, "windEnabled");
}

void retrieveHDRUniformLocations() {
    hdrShader.useShaderProgram();

	hdrUniforms.hdrBuffer = glGetUniformLocation(hdrShader.shaderProgram, "hdrBuffer");
	hdrUniforms.bloomBlur = glGetUniformLocation(hdrShader.shaderProgram, "bloomBlur");
	hdrUniforms.bloom = glGetUniformLocation(hdrShader.shaderProgram, "bloom");
	hdrUniforms.exposure = glGetUniformLocation(hdrShader.shaderProgram, "exposure");
}

void retrieveBlurUniformLocations() {
    blurShader.useShaderProgram();

	blurUniforms.horizontal = glGetUniformLocation(blurShader.shaderProgram, "horizontal");
}

void retrieveSkyboxUniformLocations() {
    skyboxShader.useShaderProgram();

	skyboxUniforms.view = glGetUniformLocation(skyboxShader.shaderProgram, "view");
	skyboxUniforms.projection = glGetUniformLocation(skyboxShader.shaderProgram, "projection");
	skyboxUniforms.viewPosition = glGetUniformLocation(skyboxShader.shaderProgram, "viewPosition");
	skyboxUniforms.daySkybox = glGetUniformLocation(skyboxShader.shaderProgram, "daySkybox");
	skyboxUniforms.nightSkybox = glGetUniformLocation(skyboxShader.shaderProgram, "nightSkybox");
	skyboxUniforms.blendFactor = glGetUniformLocation(skyboxShader.shaderProgram, "blendFactor");
}

//respawners for fire particles

void respawnFireParticle(FireParticle& p)
{
    float angle = glm::linearRand(0.0f, 2.f * 3.14159f);
    float r = glm::linearRand(0.0f, spawnRadius);
    float xOff = r * cos(angle);
    float zOff = r * sin(angle);
    float yOff = glm::linearRand(0.01f, 0.03f);

    p.position = pointLight.position + glm::vec3(xOff, yOff, zOff);

    p.velocity = glm::vec3(
        glm::linearRand(minHorizontalVel, maxHorizontalVel),
        glm::linearRand(minUpVelocity, maxUpVelocity),
        glm::linearRand(minHorizontalVel, maxHorizontalVel)
    );

    p.life = glm::linearRand(2.0f, 4.0f);
    p.initialLife = p.life;
    p.color = glm::vec4(1.0f);
    p.size = glm::linearRand(0.1f, 0.18f);
    p.rotation = glm::linearRand(0.0f, 360.0f);
    p.rotationSpeed = glm::linearRand(-60.0f, 60.0f);
    p.type = 0;

    p.textureIndex = glm::linearRand(0, 6);
}

void respawnSmokeParticle(FireParticle& p)
{
    float angle = glm::linearRand(0.0f, 2.f * 3.14159f);
    float r = glm::linearRand(0.0f, spawnRadius * 0.6f);
    float xOff = r * cos(angle);
    float zOff = r * sin(angle);
    float yOff = glm::linearRand(0.7f, 1.0f);

    p.position = pointLight.position + glm::vec3(xOff, yOff, zOff);

    p.velocity = glm::vec3(
        glm::linearRand(-0.1f, 0.1f),
        glm::linearRand(0.3f, 0.6f),
        glm::linearRand(-0.1f, 0.1f)
    );;

    p.life = glm::linearRand(0.6f, 1.2f);
    p.initialLife = p.life;
    p.color = glm::vec4(1.0f);

    p.size = glm::linearRand(0.2f, 0.35f);

    p.rotation = glm::linearRand(0.0f, 360.0f);
    p.rotationSpeed = glm::linearRand(-20.0f, 20.0f);
    p.type = 1;

    p.textureIndex = glm::linearRand(0, 6);
}

void respawnEmberParticle(FireParticle& p)
{
    float angle = glm::linearRand(0.0f, 2.f * 3.14159f);
    float r = glm::linearRand(0.0f, spawnRadius * 0.4f);
    float xOff = r * cos(angle);
    float zOff = r * sin(angle);
    float yOff = glm::linearRand(0.03f, 0.06f);

    p.position = pointLight.position + glm::vec3(xOff, yOff, zOff);

    float upMin = 2.5f;
    float upMax = 3.5f;

    if (glm::linearRand(0.0f, 1.0f) < 0.1f) {
        upMax = 5.0f;
    }

    p.velocity = glm::vec3(
        glm::linearRand(-0.2f, 0.2f),
        glm::linearRand(upMin, upMax),
        glm::linearRand(-0.2f, 0.2f)
    );

    p.life = glm::linearRand(0.4f, 1.5f);
    p.initialLife = p.life;
    p.color = glm::vec4(1.0f);
    p.size = glm::linearRand(0.07f, 0.12f);
    p.rotation = glm::linearRand(0.0f, 360.0f);
    p.rotationSpeed = glm::linearRand(-80.0f, 80.0f);
    p.type = 2;

    p.textureIndex = glm::linearRand(0, 6);
}

//initializers
void initOpenGLWindow() {
    myWindow.Create(3200, 2000, "OpenGL Forest");
    //glfwSetWindowUserPointer(myWindow.getWindow(), &myWindow);

}

void initOpenGLState() {
    glClearColor(0.7f, 0.7f, 0.7f, 1.0f);
    glViewport(0, 0, myWindow.getWindowDimensions().width, myWindow.getWindowDimensions().height);
    //glEnable(GL_FRAMEBUFFER_SRGB);
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);
    glEnable(GL_CLIP_DISTANCE0);
    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);
    glFrontFace(GL_CCW);
   // glfwSwapInterval(0);
}

void initModels() {
    forest.LoadModel("models/forest/Evergreen_Forest_2_naked.obj");

    std::vector<std::string> dayFaces = {
        "faces/right.jpg", "faces/left.jpg", "faces/top.jpg", "faces/bottom.jpg", "faces/front.jpg", "faces/back.jpg"
    };
    std::vector<std::string> nightFaces = {
        "faces/nightRight.png", "faces/nightLeft.png", "faces/nightTop.png",
        "faces/nightBottom.png", "faces/nightFront.png", "faces/nightBack.png"
    };

    daySkybox = new Skybox(dayFaces);
    nightSkybox = new Skybox(nightFaces);
}

void initShaders() {
    myBasicShader.loadShader(
        "shaders/basic.vert",
        "shaders/basic.frag",
        gps::ShaderType::MAIN_SHADER
    );

    // Load SKYBOX_SHADER
    skyboxShader.loadShader(
        "shaders/skybox.vert",
        "shaders/skybox.frag",
        gps::ShaderType::SKYBOX_SHADER
    );

    // Load SHADOW_SHADER
    shadowShader.loadShader(
        "shaders/depth.vert",
        "shaders/depth.frag",
        gps::ShaderType::SHADOW_SHADER
    );

    // Load POINT_SHADOW_SHADER with Geometry Shader
    pointShadowShader.loadShader(
        "shaders/pointdepth.vert",
        "shaders/pointdepth.frag",
        "shaders/pointdepth.geom",
        gps::ShaderType::SHADOW_SHADER
    );

    // Load HEAD_SHADOW_SHADER
    headShadowShader.loadShader(
        "shaders/headDepth.vert",
        "shaders/headDepth.frag",
        gps::ShaderType::SHADOW_SHADER
    );

    // Load RAIN_SHADER with Geometry Shader
    rainShader.loadShader(
        "shaders/rain.vert",
        "shaders/rain.frag",
        "shaders/rain.geom",
        gps::ShaderType::RAIN_SHADER
    );

    // Load HDR_SHADER
    hdrShader.loadShader(
        "shaders/hdr.vert",
        "shaders/hdr.frag",
        gps::ShaderType::HDR_SHADER
    );

    // Load FIRE_SHADER
    fireShader.loadShader(
        "shaders/fire.vert",
        "shaders/fire.frag",
        gps::ShaderType::FIRE_SHADER
    );

    // Load BLUR_SHADER
    blurShader.loadShader(
        "shaders/blur.vert",
        "shaders/blur.frag",
        gps::ShaderType::BLUR_SHADER
    );

    retrieveRainUniformLocations();
    retrieveBasicUniformLocations();
    retrieveFireUniformLocations();
    retrieveShadowUniformLocations();
    retrievePointShadowUniformLocations();
    retrieveHeadShadowUniformLocations();
    retrieveHDRUniformLocations();
    retrieveBlurUniformLocations();
    retrieveSkyboxUniformLocations();
}

void initRainUniforms() {
    rainShader.useShaderProgram();
    view = myCamera.getViewMatrix();
    projection = glm::perspective(glm::radians(myCamera.getFov()),
        (float)myWindow.getWindowDimensions().width / (float)myWindow.getWindowDimensions().height,
        0.1f, 300.0f);
    glUniformMatrix4fv(rainUniforms.view, 1, GL_FALSE, glm::value_ptr(view));
    glUniformMatrix4fv(rainUniforms.projection, 1, GL_FALSE, glm::value_ptr(projection));

    glUniform3fv(rainUniforms.viewPosition, 1, glm::value_ptr(myCamera.getPosition()));

    glUniform3fv(rainUniforms.dirLight.direction, 1, glm::value_ptr(dirLight.direction));
    glUniform3fv(rainUniforms.dirLight.ambient, 1, glm::value_ptr(dirLight.ambient));
    glUniform3fv(rainUniforms.dirLight.diffuse, 1, glm::value_ptr(dirLight.diffuse));
    glUniform3fv(rainUniforms.dirLight.specular, 1, glm::value_ptr(dirLight.specular));
    glUniform3fv(rainUniforms.dirLight.color, 1, glm::value_ptr(dirLight.color));
    glUniform1i(rainUniforms.dirLight.enabled, dirLight.enabled);

    glUniform3fv(rainUniforms.flashLight.direction, 1, glm::value_ptr(flashLight.direction));
    glUniform3fv(rainUniforms.flashLight.ambient, 1, glm::value_ptr(flashLight.ambient));
    glUniform3fv(rainUniforms.flashLight.diffuse, 1, glm::value_ptr(flashLight.diffuse));
    glUniform3fv(rainUniforms.flashLight.specular, 1, glm::value_ptr(flashLight.specular));
    glUniform3fv(rainUniforms.flashLight.color, 1, glm::value_ptr(flashLight.color));
    glUniform1i(rainUniforms.flashLight.enabled, flashLight.enabled);

    glUniform3fv(rainUniforms.pointLight.position, 1, glm::value_ptr(pointLight.position));
    glUniform3fv(rainUniforms.pointLight.ambient, 1, glm::value_ptr(pointLight.ambient));
    glUniform3fv(rainUniforms.pointLight.diffuse, 1, glm::value_ptr(pointLight.diffuse));
    glUniform3fv(rainUniforms.pointLight.specular, 1, glm::value_ptr(pointLight.specular));
    glUniform3fv(rainUniforms.pointLight.color, 1, glm::value_ptr(pointLight.color));
    glUniform1i(rainUniforms.pointLight.enabled, pointLight.enabled);
    glUniform1f(rainUniforms.pointLight.constant, pointLight.constant);
    glUniform1f(rainUniforms.pointLight.linear, pointLight.linear);
    glUniform1f(rainUniforms.pointLight.quadratic, pointLight.quadratic);

    glUniform3fv(rainUniforms.spotLight.position, 1, glm::value_ptr(spotLight.position));
    glUniform3fv(rainUniforms.spotLight.direction, 1, glm::value_ptr(spotLight.direction));
    glUniform1f(rainUniforms.spotLight.cutOff, spotLight.cutOff);
    glUniform1f(rainUniforms.spotLight.outerCutOff, spotLight.outerCutOff);
    glUniform3fv(rainUniforms.spotLight.ambient, 1, glm::value_ptr(spotLight.ambient));
    glUniform3fv(rainUniforms.spotLight.diffuse, 1, glm::value_ptr(spotLight.diffuse));
    glUniform3fv(rainUniforms.spotLight.specular, 1, glm::value_ptr(spotLight.specular));
    glUniform3fv(rainUniforms.spotLight.color, 1, glm::value_ptr(spotLight.color));
    glUniform1i(rainUniforms.spotLight.enabled, spotLight.enabled);

    glUniform3fv(rainUniforms.leftHeadlight.position, 1, glm::value_ptr(leftHeadlight.position));
    glUniform3fv(rainUniforms.leftHeadlight.direction, 1, glm::value_ptr(leftHeadlight.direction));
    glUniform1f(rainUniforms.leftHeadlight.cutOff, leftHeadlight.cutOff);
    glUniform1f(rainUniforms.leftHeadlight.outerCutOff, leftHeadlight.outerCutOff);
    glUniform3fv(rainUniforms.leftHeadlight.ambient, 1, glm::value_ptr(leftHeadlight.ambient));
    glUniform3fv(rainUniforms.leftHeadlight.diffuse, 1, glm::value_ptr(leftHeadlight.diffuse));
    glUniform3fv(rainUniforms.leftHeadlight.specular, 1, glm::value_ptr(leftHeadlight.specular));
    glUniform3fv(rainUniforms.leftHeadlight.color, 1, glm::value_ptr(leftHeadlight.color));
    glUniform1i(rainUniforms.leftHeadlight.enabled, leftHeadlight.enabled);

    glUniform3fv(rainUniforms.rightHeadlight.position, 1, glm::value_ptr(rightHeadlight.position));
    glUniform3fv(rainUniforms.rightHeadlight.direction, 1, glm::value_ptr(rightHeadlight.direction));
    glUniform1f(rainUniforms.rightHeadlight.cutOff, rightHeadlight.cutOff);
    glUniform1f(rainUniforms.rightHeadlight.outerCutOff, rightHeadlight.outerCutOff);
    glUniform3fv(rainUniforms.rightHeadlight.ambient, 1, glm::value_ptr(rightHeadlight.ambient));
    glUniform3fv(rainUniforms.rightHeadlight.diffuse, 1, glm::value_ptr(rightHeadlight.diffuse));
    glUniform3fv(rainUniforms.rightHeadlight.specular, 1, glm::value_ptr(rightHeadlight.specular));
    glUniform3fv(rainUniforms.rightHeadlight.color, 1, glm::value_ptr(rightHeadlight.color));
    glUniform1i(rainUniforms.rightHeadlight.enabled, rightHeadlight.enabled);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_CUBE_MAP, isDay ? daySkybox->getCubemapTexture() : nightSkybox->getCubemapTexture());
    glUniform1i(rainUniforms.environmentMap, 0);

    glUniform1f(rainUniforms.shininess, 32.0f);

    glUniform1f(rainUniforms.maxDistance, 50.0f);
    glUniform1f(rainUniforms.motionBlurIntensity, 0.7f);
}

void initUniforms() {
    myBasicShader.useShaderProgram();

    model = glm::rotate(glm::mat4(1.0f), glm::radians(angle), glm::vec3(0.0f, 1.0f, 0.0f));

    view = myCamera.getViewMatrix();

    glUniformMatrix4fv(basicUniforms.view, 1, GL_FALSE, glm::value_ptr(view));

    projection = glm::perspective(glm::radians(myCamera.getFov()),
        (float)myWindow.getWindowDimensions().width / (float)myWindow.getWindowDimensions().height,
        0.1f, 300.0f);

    glUniformMatrix4fv(basicUniforms.projection, 1, GL_FALSE, glm::value_ptr(projection));

    glUniform3fv(basicUniforms.viewPosition, 1, glm::value_ptr(myCamera.getPosition()));

    glUniform4fv(basicUniforms.clipPlane, 1, glm::value_ptr(glm::vec4(0.0f, -1.0f, 0.0f, 15.0f)));

    glUniform1f(basicUniforms.u_Time, u_Time);
    glUniform3fv(basicUniforms.u_WindDirection, 1, glm::value_ptr(u_WindDirection));
    glUniform1f(basicUniforms.u_WindStrength, u_WindStrength);
	glUniform1f(basicUniforms.u_GustSize, gustSize);
	glUniform1f(basicUniforms.u_GustSpeed, gustSpeed);
	glUniform1f(basicUniforms.u_WindWaveLength, windWaveLength);


    glUniform3fv(basicUniforms.dirLight.direction, 1, glm::value_ptr(dirLight.direction));
    glUniform3fv(basicUniforms.dirLight.ambient, 1, glm::value_ptr(dirLight.ambient));
    glUniform3fv(basicUniforms.dirLight.diffuse, 1, glm::value_ptr(dirLight.diffuse));
    glUniform3fv(basicUniforms.dirLight.specular, 1, glm::value_ptr(dirLight.specular));
    glUniform3fv(basicUniforms.dirLight.color, 1, glm::value_ptr(dirLight.color));
    glUniform1i(basicUniforms.dirLight.enabled, dirLight.enabled);

    glUniform3fv(basicUniforms.flashLight.direction, 1, glm::value_ptr(flashLight.direction));
    glUniform3fv(basicUniforms.flashLight.ambient, 1, glm::value_ptr(flashLight.ambient));
    glUniform3fv(basicUniforms.flashLight.diffuse, 1, glm::value_ptr(flashLight.diffuse));
    glUniform3fv(basicUniforms.flashLight.specular, 1, glm::value_ptr(flashLight.specular));
    glUniform3fv(basicUniforms.flashLight.color, 1, glm::value_ptr(flashLight.color));
    glUniform1i(basicUniforms.flashLight.enabled, flashLight.enabled);

    glUniform3fv(basicUniforms.pointLight.position, 1, glm::value_ptr(pointLight.position));
    glUniform3fv(basicUniforms.pointLight.ambient, 1, glm::value_ptr(pointLight.ambient));
    glUniform3fv(basicUniforms.pointLight.diffuse, 1, glm::value_ptr(pointLight.diffuse));
    glUniform3fv(basicUniforms.pointLight.specular, 1, glm::value_ptr(pointLight.specular));
    glUniform3fv(basicUniforms.pointLight.color, 1, glm::value_ptr(pointLight.color));
    glUniform1i(basicUniforms.pointLight.enabled, pointLight.enabled);
    glUniform1f(basicUniforms.pointLight.constant, pointLight.constant);
    glUniform1f(basicUniforms.pointLight.linear, pointLight.linear);
    glUniform1f(basicUniforms.pointLight.quadratic, pointLight.quadratic);

    glUniform3fv(basicUniforms.spotLight.position, 1, glm::value_ptr(spotLight.position));
    glUniform3fv(basicUniforms.spotLight.direction, 1, glm::value_ptr(spotLight.direction));
    glUniform1f(basicUniforms.spotLight.cutOff, spotLight.cutOff);
    glUniform1f(basicUniforms.spotLight.outerCutOff, spotLight.outerCutOff);
    glUniform3fv(basicUniforms.spotLight.ambient, 1, glm::value_ptr(spotLight.ambient));
    glUniform3fv(basicUniforms.spotLight.diffuse, 1, glm::value_ptr(spotLight.diffuse));
    glUniform3fv(basicUniforms.spotLight.specular, 1, glm::value_ptr(spotLight.specular));
    glUniform3fv(basicUniforms.spotLight.color, 1, glm::value_ptr(spotLight.color));
    glUniform1i(basicUniforms.spotLight.enabled, spotLight.enabled);


    glUniform3fv(basicUniforms.leftHeadlight.position, 1, glm::value_ptr(leftHeadlight.position));
    glUniform3fv(basicUniforms.leftHeadlight.direction, 1, glm::value_ptr(leftHeadlight.direction));
    glUniform3fv(basicUniforms.leftHeadlight.ambient, 1, glm::value_ptr(leftHeadlight.ambient));
    glUniform3fv(basicUniforms.leftHeadlight.diffuse, 1, glm::value_ptr(leftHeadlight.diffuse));
    glUniform3fv(basicUniforms.leftHeadlight.specular, 1, glm::value_ptr(leftHeadlight.specular));
    glUniform3fv(basicUniforms.leftHeadlight.color, 1, glm::value_ptr(leftHeadlight.color));
    glUniform1f(basicUniforms.leftHeadlight.cutOff, leftHeadlight.cutOff);
    glUniform1f(basicUniforms.leftHeadlight.outerCutOff, leftHeadlight.outerCutOff);
    glUniform1i(basicUniforms.leftHeadlight.enabled, leftHeadlight.enabled);

    glUniform3fv(basicUniforms.rightHeadlight.position, 1, glm::value_ptr(rightHeadlight.position));
    glUniform3fv(basicUniforms.rightHeadlight.direction, 1, glm::value_ptr(rightHeadlight.direction));
    glUniform3fv(basicUniforms.rightHeadlight.ambient, 1, glm::value_ptr(rightHeadlight.ambient));
    glUniform3fv(basicUniforms.rightHeadlight.diffuse, 1, glm::value_ptr(rightHeadlight.diffuse));
    glUniform3fv(basicUniforms.rightHeadlight.specular, 1, glm::value_ptr(rightHeadlight.specular));
    glUniform3fv(basicUniforms.rightHeadlight.color, 1, glm::value_ptr(rightHeadlight.color));
    glUniform1f(basicUniforms.rightHeadlight.cutOff, rightHeadlight.cutOff);
    glUniform1f(basicUniforms.rightHeadlight.outerCutOff, rightHeadlight.outerCutOff);
    glUniform1i(basicUniforms.rightHeadlight.enabled, rightHeadlight.enabled);

    glUniform1i(basicUniforms.useNormalMapping, useNormalMapping);
    glUniform1i(basicUniforms.rainEnabled, rainEnabled);
    glUniform1i(basicUniforms.windEnabled, windEnabled);

    glUniform1i(basicUniforms.fogEnabled, fogEnabled);
    glUniform3fv(basicUniforms.fogColor, 1, glm::value_ptr(glm::vec3(0.5f, 0.5f, 0.5f)));
	glUniform1f(basicUniforms.gFogEnd, 50.0f);
	glUniform1f(basicUniforms.gLayeredFogTop, 20.0f);
	glUniform1f(basicUniforms.gExpFogDensity, 0.02f);
	glUniform1f(basicUniforms.gFogTime, fogTime);

    glUniform1f(basicUniforms.globalLightIntensity, 1.0f);

    rainShader.useShaderProgram();
    initRainUniforms();


}

void initShadowMapping() {
    glGenFramebuffers(1, &shadowMapFBO);
    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
        std::cerr << "Framebuffer is not complete!" << std::endl;

    glGenTextures(1, &depthMap);
    glBindTexture(GL_TEXTURE_2D, depthMap);
    if (!glIsTexture(depthMap))
        std::cerr << "Error: depthMap texture is invalid!" << std::endl;

    glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, SHADOW_WIDTH, SHADOW_HEIGHT, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
    float borderColor[] = { 1.0f, 1.0f, 1.0f, 1.0f };
    glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, borderColor);

    glBindFramebuffer(GL_FRAMEBUFFER, shadowMapFBO);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, depthMap, 0);
    glDrawBuffer(GL_NONE);
    glReadBuffer(GL_NONE);
    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
        std::cerr << "Framebuffer is not complete!" << std::endl;
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void initPointLightShadowMapping() {
    glGenFramebuffers(1, &pointLightFBO);
    glGenTextures(1, &depthCubemap);

    glBindTexture(GL_TEXTURE_CUBE_MAP, depthCubemap);
    for (unsigned int i = 0; i < 6; ++i) {
        glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_DEPTH_COMPONENT, POINT_SHADOW_WIDTH, POINT_SHADOW_HEIGHT, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
    }
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

    glBindFramebuffer(GL_FRAMEBUFFER, pointLightFBO);
    glFramebufferTexture(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, depthCubemap, 0);
    glDrawBuffer(GL_NONE);
    glReadBuffer(GL_NONE);

    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
        std::cerr << "Point Light Framebuffer is not complete!" << std::endl;
    }
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void initHeadlightShadowMapping(GLuint& FBO, GLuint& depthMap) {
    glGenFramebuffers(1, &FBO);
    glGenTextures(1, &depthMap);

    glBindTexture(GL_TEXTURE_2D, depthMap);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT,
        SPOT_LIGHT_SHADOW_WIDTH, SPOT_LIGHT_SHADOW_HEIGHT,
        0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
    float borderColor[] = { 1.0f, 1.0f, 1.0f, 1.0f };
    glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, borderColor);

    glBindFramebuffer(GL_FRAMEBUFFER, FBO);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, depthMap, 0);
    glDrawBuffer(GL_NONE);
    glReadBuffer(GL_NONE);

    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
        std::cerr << "Headlight Framebuffer is not complete!" << std::endl;
    }
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void initRain() {
    raindrops.resize(NUM_RAINDROPS);
    raindropsVelocity.resize(NUM_RAINDROPS);
    raindropParams.resize(NUM_RAINDROPS);

    for (int i = 0; i < NUM_RAINDROPS; i++) {
        float x = rainMinX + static_cast<float>(rand()) / (static_cast<float>(RAND_MAX / (rainMaxX - rainMinX)));
        float z = rainMinZ + static_cast<float>(rand()) / (static_cast<float>(RAND_MAX / (rainMaxZ - rainMinZ)));
        float y = rainTopY + static_cast<float>(rand()) / (static_cast<float>(RAND_MAX / 5.0f));

        float lengthFactor = 0.5f + static_cast<float>(rand()) / RAND_MAX * 2.0f;
        float speedFactor = 0.5f + static_cast<float>(rand()) / RAND_MAX * 2.0f;

        raindrops[i] = glm::vec3(x, y, z);
        raindropsVelocity[i] = glm::vec3(0.0f, GRAVITY * speedFactor, 0.0f);
        raindropParams[i] = glm::vec2(lengthFactor, speedFactor);
    }

    glGenVertexArrays(1, &rainVAO);
    glBindVertexArray(rainVAO);

    glGenBuffers(1, &rainVBO);
    glBindBuffer(GL_ARRAY_BUFFER, rainVBO);
    glBufferData(GL_ARRAY_BUFFER, NUM_RAINDROPS * sizeof(glm::vec3), &raindrops[0], GL_DYNAMIC_DRAW);

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(glm::vec3), (void*)0);

    glGenBuffers(1, &rainParamsVBO);
    glBindBuffer(GL_ARRAY_BUFFER, rainParamsVBO);
    glBufferData(GL_ARRAY_BUFFER, NUM_RAINDROPS * sizeof(glm::vec2), &raindropParams[0], GL_STATIC_DRAW);

    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(glm::vec2), (void*)0);

    glBindVertexArray(0);
}

void initHDRFramebuffer() {
    glGenFramebuffers(1, &hdrFBO);
    glBindFramebuffer(GL_FRAMEBUFFER, hdrFBO);

    glGenTextures(2, colorBuffers);
    for (unsigned int i = 0; i < 2; i++)
    {
        glBindTexture(GL_TEXTURE_2D, colorBuffers[i]);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F,
            myWindow.getWindowDimensions().width,
            myWindow.getWindowDimensions().height,
            0, GL_RGBA, GL_FLOAT, NULL);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

        glFramebufferTexture2D(GL_FRAMEBUFFER,
            GL_COLOR_ATTACHMENT0 + i,
            GL_TEXTURE_2D,
            colorBuffers[i], 0);
    }

    glGenRenderbuffers(1, &rbo);
    glBindRenderbuffer(GL_RENDERBUFFER, rbo);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT,
        myWindow.getWindowDimensions().width,
        myWindow.getWindowDimensions().height);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT,
        GL_RENDERBUFFER, rbo);

    GLuint attachments[2] = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1 };
    glDrawBuffers(2, attachments);

    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
        std::cout << "HDR Framebuffer not complete!" << std::endl;

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void initBloomBuffers()
{
    glGenFramebuffers(2, pingpongFBO);
    glGenTextures(2, pingpongColorbuffers);

    for (unsigned int i = 0; i < 2; i++)
    {
        glBindFramebuffer(GL_FRAMEBUFFER, pingpongFBO[i]);
        glBindTexture(GL_TEXTURE_2D, pingpongColorbuffers[i]);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F,
            myWindow.getWindowDimensions().width,
            myWindow.getWindowDimensions().height,
            0, GL_RGBA, GL_FLOAT, NULL);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
            GL_TEXTURE_2D, pingpongColorbuffers[i], 0);

        if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
            std::cout << "Ping-Pong FBO " << i << " not complete!" << std::endl;
    }
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}
void initFire()
{

    std::vector<std::string> firePaths = {
        "models/forest/textures_512/fire1.png",
        "models/forest/textures_512/fire2.png",
        "models/forest/textures_512/fire3.png",
        "models/forest/textures_512/fire4.png",
        "models/forest/textures_512/fire5.png",
        "models/forest/textures_512/fire6.png",
		"models/forest/textures_512/fire7.png",
		"models/forest/textures_512/fire8.png",
		"models/forest/textures_512/fire9.png",
		"models/forest/textures_512/fire10.png"
    };

    fireTextureArray = LoadFireTextureArray(firePaths);

    fireParticles.reserve(MAX_TOTAL_PARTICLES);

    for (int i = 0; i < MAX_FIRE_PARTICLES; ++i) {
        FireParticle p;
        respawnFireParticle(p);
        fireParticles.push_back(p);
    }

    for (int i = 0; i < MAX_SMOKE_PARTICLES; ++i) {
        FireParticle p;
        respawnSmokeParticle(p);
        fireParticles.push_back(p);
    }

    for (int i = 0; i < MAX_EMBER_PARTICLES; ++i) {
        FireParticle p;
        respawnEmberParticle(p);
        fireParticles.push_back(p);
    }

    glGenVertexArrays(1, &quad2VAO);
    glGenBuffers(1, &quad2VBO);
    glBindVertexArray(quad2VAO);

    glBindBuffer(GL_ARRAY_BUFFER, quad2VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(quad2Vertices), quad2Vertices, GL_STATIC_DRAW);

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE,
        4 * sizeof(GLfloat), (void*)0);

    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE,
        4 * sizeof(GLfloat), (void*)(2 * sizeof(GLfloat)));

    glGenBuffers(1, &instanceVBO);
    glBindBuffer(GL_ARRAY_BUFFER, instanceVBO);
    glBufferData(GL_ARRAY_BUFFER, MAX_TOTAL_PARTICLES * sizeof(FireParticle),
        &fireParticles[0], GL_STREAM_DRAW);

    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE,
        sizeof(FireParticle),
        (void*)offsetof(FireParticle, position));
    glVertexAttribDivisor(2, 1);

    glEnableVertexAttribArray(3);
    glVertexAttribPointer(3, 4, GL_FLOAT, GL_FALSE,
        sizeof(FireParticle),
        (void*)offsetof(FireParticle, color));
    glVertexAttribDivisor(3, 1);

    glEnableVertexAttribArray(4);
    glVertexAttribPointer(4, 1, GL_FLOAT, GL_FALSE,
        sizeof(FireParticle),
        (void*)offsetof(FireParticle, size));
    glVertexAttribDivisor(4, 1);

    glEnableVertexAttribArray(5);
    glVertexAttribPointer(5, 1, GL_FLOAT, GL_FALSE,
        sizeof(FireParticle),
        (void*)offsetof(FireParticle, rotation));
    glVertexAttribDivisor(5, 1);

    glEnableVertexAttribArray(6);
    glVertexAttribIPointer(6, 1, GL_INT,
        sizeof(FireParticle),
        (void*)offsetof(FireParticle, textureIndex));
    glVertexAttribDivisor(6, 1);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
}

//updaters
void updateTour() {
    if (!isTourActive || currentWaypoint >= keyLocations.size() - 3) return;
    size_t idx = currentWaypoint;
    if (idx + 3 >= keyLocations.size()) {
        std::cerr << "Error: Attempting to access keyLocations out of bounds." << std::endl;
        isTourActive = false;
        return;
    }
    const Location& P0 = keyLocations[idx];
    const Location& P1 = keyLocations[idx + 1];
    const Location& P2 = keyLocations[idx + 2];
    const Location& P3 = keyLocations[idx + 3];

    float easedT = easeInOut(t);

    glm::vec3 interpolatedPos = catmullRomInterpolate(P0.position, P1.position, P2.position, P3.position, easedT);
    glm::vec3 interpolatedTarget = catmullRomInterpolate(P0.target, P1.target, P2.target, P3.target, easedT);
    glm::vec3 interpolatedUp = catmullRomInterpolate(P0.up, P1.up, P2.up, P3.up, easedT);
    glCheckError();

    myCamera.setPosition(interpolatedPos);
    myCamera.setTarget(interpolatedTarget);
    myCamera.setUpDirection(interpolatedUp);

    glm::mat4 view = myCamera.getViewMatrix();
    glCheckError();

    myBasicShader.useShaderProgram();
    glCheckError();
    glUniformMatrix4fv(basicUniforms.view, 1, GL_FALSE, glm::value_ptr(view));
    glCheckError();

    t += tIncrement;
    if (t > 1.0f) {
        t = 0.0f;
        currentWaypoint++;
        if (currentWaypoint >= keyLocations.size() - 3) {
            isTourActive = false;
            std::cout << "Tour completed" << std::endl;
        }
    }
}

void updateGlobalLightIntensity(float deltaTime) {
    for (auto it = activePulses.begin(); it != activePulses.end(); ) {
        it->timer -= deltaTime;
        if (it->timer <= 0.0f) {
            it = activePulses.erase(it);
        }
        else {
            ++it;
        }
    }

    globalLightIntensity = 1.0f;
    for (const auto& pulse : activePulses) {
        globalLightIntensity += pulse.intensity;
    }

    globalLightIntensity = glm::clamp(globalLightIntensity, 1.0f, 15.0f);
    myBasicShader.useShaderProgram();
    glUniform1f(basicUniforms.globalLightIntensity, globalLightIntensity);
}

void updatePointLightIntensity(PointLight& pointLight, float deltaTime) {

    pointLight.flickerPhase += 2.0f * glm::pi<float>() * FLICKER_BASE_FREQUENCY * deltaTime;
    if (pointLight.flickerPhase > 2.0f * glm::pi<float>()) {
        pointLight.flickerPhase -= 2.0f * glm::pi<float>();
    }

    float ambientVariation = sin(pointLight.flickerPhase) * FLICKER_AMPLITUDE_AMBIENT;
    float diffuseVariation = sin(pointLight.flickerPhase * 1.5f) * FLICKER_AMPLITUDE_DIFFUSE;
    float specularVariation = sin(pointLight.flickerPhase * 2.0f) * FLICKER_AMPLITUDE_SPECULAR;

    ambientVariation += ((rand() % 100) / 1000.0f - 0.05f) * FLICKER_RANDOMNESS;
    diffuseVariation += ((rand() % 100) / 1000.0f - 0.05f) * FLICKER_RANDOMNESS;
    specularVariation += ((rand() % 100) / 1000.0f - 0.05f) * FLICKER_RANDOMNESS;

    pointLight.ambient = glm::vec3(
        glm::clamp(0.15f + ambientVariation, 0.10f, 0.20f),
        glm::clamp(0.07f + ambientVariation * 0.5f, 0.03f, 0.10f),
        glm::clamp(0.02f + ambientVariation * 0.2f, 0.00f, 0.05f)
    );

    pointLight.diffuse = glm::vec3(
        glm::clamp(1.0f + diffuseVariation, 0.9f, 1.1f),
        glm::clamp(0.55f + diffuseVariation * 0.5f, 0.5f, 0.6f),
        glm::clamp(0.2f + diffuseVariation * 0.3f, 0.15f, 0.25f)
    );

    pointLight.specular = glm::vec3(
        glm::clamp(1.0f + specularVariation, 0.9f, 1.1f),
        glm::clamp(0.9f + specularVariation * 0.5f, 0.85f, 0.95f),
        glm::clamp(0.5f + specularVariation * 0.3f, 0.45f, 0.55f)
    );

    pointLight.constant = glm::clamp(1.0f + sin(pointLight.flickerPhase * 0.5f) * 0.05f, 0.95f, 1.05f);
    pointLight.linear = glm::clamp(0.09f + sin(pointLight.flickerPhase * 0.7f) * 0.01f, 0.08f, 0.10f);
    pointLight.quadratic = glm::clamp(0.032f + sin(pointLight.flickerPhase * 0.9f) * 0.005f, 0.030f, 0.035f);
}



void updateRain(float deltaTime) {
    if (!rainEnabled) return;
    for (int i = 0; i < NUM_RAINDROPS; i++) {
        raindropsVelocity[i].y += GRAVITY * deltaTime;

        if (raindropsVelocity[i].y < TERMINAL_VELOCITY) {
            raindropsVelocity[i].y = TERMINAL_VELOCITY;
        }

        raindrops[i] += raindropsVelocity[i] * deltaTime;

        if (raindrops[i].y < rainBottomY) {
            float x = rainMinX + static_cast<float>(rand()) / (static_cast<float>(RAND_MAX / (rainMaxX - rainMinX)));
            float z = rainMinZ + static_cast<float>(rand()) / (static_cast<float>(RAND_MAX / (rainMaxZ - rainMinZ)));
            float y = rainTopY + static_cast<float>(rand()) / (static_cast<float>(RAND_MAX / 5.0f));

            float lengthFactor = 0.5f + static_cast<float>(rand()) / RAND_MAX * 2.0f;
            float speedFactor = 0.5f + static_cast<float>(rand()) / RAND_MAX * 2.0f;

            raindrops[i] = glm::vec3(x, y, z);
            raindropsVelocity[i] = glm::vec3(0.0f, GRAVITY * speedFactor, 0.0f);
            raindropParams[i] = glm::vec2(lengthFactor, speedFactor);
        }
    }

    glBindBuffer(GL_ARRAY_BUFFER, rainVBO);
    glBufferSubData(GL_ARRAY_BUFFER, 0, NUM_RAINDROPS * sizeof(glm::vec3), &raindrops[0]);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
}

void updateSpotlightRange(float fovDegrees) {

    float innerAngle = glm::clamp(fovDegrees * 0.3f, 5.0f, 30.0f);
    float outerAngle = glm::clamp(fovDegrees * 0.4f, innerAngle + 5.0f, 45.0f);

    spotLight.cutOff = glm::cos(glm::radians(innerAngle));
    spotLight.outerCutOff = glm::cos(glm::radians(outerAngle));

    myBasicShader.useShaderProgram();
    glUniform1f(basicUniforms.spotLight.cutOff, spotLight.cutOff);
    glUniform1f(basicUniforms.spotLight.outerCutOff, spotLight.outerCutOff);

    std::cout << "Spotlight angles updated based on FOV " << fovDegrees << "°: "
        << "Inner Angle = " << innerAngle << "°, "
        << "Outer Angle = " << outerAngle << "°" << std::endl;
}


glm::vec4 multiStageColor(const FireParticle& p)
{
    float lifeRatio = p.life / p.initialLife;

    switch (p.type) {
    case 0: {
        glm::vec3 cStart = glm::vec3(1.0f, 0.9f, 0.6f);
        glm::vec3 cMid = glm::vec3(1.0f, 0.5f, 0.1f);
        glm::vec3 cLate = glm::vec3(0.6f, 0.0f, 0.0f);
        glm::vec3 cEnd = glm::vec3(0.15f, 0.0f, 0.0f);

        if (lifeRatio < 0.4f) {
            float t = lifeRatio / 0.4f;
            return glm::vec4(glm::mix(cStart, cMid, t), 1.0f);
        }
        else if (lifeRatio < 0.8f) {
            float t = (lifeRatio - 0.4f) / 0.4f;
            return glm::vec4(glm::mix(cMid, cLate, t), 1.0f);
        }
        else {
            float t = (lifeRatio - 0.8f) / 0.2f;
            glm::vec3 c = glm::mix(cLate, cEnd, t);
            float alphaFade = 1.0f - 0.5f * t;
            return glm::vec4(c, alphaFade);
        }
    }
    case 1: {
        glm::vec3 c0 = glm::vec3(0.1f, 0.1f, 0.1f);
        glm::vec3 c1 = glm::vec3(0.45f, 0.45f, 0.45f);
        glm::vec3 c = glm::mix(c0, c1, 1.0f - lifeRatio);

        float alpha = lifeRatio * 0.7f;
        return glm::vec4(c, alpha);
    }
    case 2: {
        glm::vec3 cStart = glm::vec3(1.0f, 0.9f, 0.4f);
        glm::vec3 cEnd = glm::vec3(1.0f, 0.2f, 0.0f);

        glm::vec3 c = glm::mix(cStart, cEnd, 1.0f - lifeRatio);
        float alpha = glm::mix(1.0f, 0.0f, 1.0f - lifeRatio);
        return glm::vec4(c, alpha);
    }
    }
    return glm::vec4(1.0f);
}


void updateFire(float deltaTime, float globalTime)
{
    for (auto& p : fireParticles)
    {
        p.life -= deltaTime;
        if (p.life <= 0.0f)
        {

            float roll = glm::linearRand(0.0f, 1.0f);
            if (roll < 0.70f) respawnFireParticle(p);
            else if (roll < 0.90f) respawnSmokeParticle(p);
            else                   respawnEmberParticle(p);
            continue;
        }

        float lifeRatio = p.life / p.initialLife;

        float swirlSpeed = 0.0f;
        if (p.type == 0) {

            swirlSpeed = (0.3f + 0.1f * sin(globalTime * 1.5f))
                + glm::linearRand(-0.05f, 0.05f);

            float heightAbove = p.position.y - pointLight.position.y;

            float topFactor = glm::smoothstep(0.2f, 0.6f, heightAbove);
            swirlSpeed += 0.2f * topFactor;

            if (glm::linearRand(0.0f, 1.0f) < 0.005f) {
                p.velocity.y += glm::linearRand(0.5f, 1.5f);
            }

            if (heightAbove > 0.4f) {
                float flickerFactor = glm::smoothstep(0.4f, 1.0f, heightAbove);
                float randomShift = glm::linearRand(-0.1f, 0.1f) * flickerFactor;
                p.color.r += randomShift;
                p.color.g += randomShift * 0.5f;
                p.color.r = glm::clamp(p.color.r, 0.0f, 1.0f);
                p.color.g = glm::clamp(p.color.g, 0.0f, 1.0f);
            }
        }
        else if (p.type == 1) {
            swirlSpeed = (0.3f + 0.2f * glm::linearRand(0.0f, 1.0f))
                + glm::linearRand(-0.05f, 0.05f);

            p.velocity.y += sin(p.position.x + p.position.z + globalTime * 2.0f)
                * 0.07f * deltaTime;
        }
        else {
            swirlSpeed = (0.25f + glm::linearRand(-0.05f, 0.05f))
                + glm::linearRand(-0.05f, 0.05f);
        }

        float swirlAngle = swirlSpeed * deltaTime;
        float c = cos(swirlAngle);
        float s = sin(swirlAngle);

        float vx = p.velocity.x * c - p.velocity.z * s;
        float vz = p.velocity.x * s + p.velocity.z * c;
        p.velocity.x = vx;
        p.velocity.z = vz;

        float noiseScale = (p.type == 0) ? 3.0f : 2.0f;
        float noiseStrength = (p.type == 0) ? 0.05f
            : ((p.type == 1) ? 0.02f : 0.03f);
        glm::vec3 noiseVec = glm::vec3(
            glm::perlin(p.position * noiseScale + glm::vec3(0.0f, 1.0f, 0.0f)),
            0.0f,
            glm::perlin(p.position * noiseScale + glm::vec3(2.0f, 1.0f, 0.0f))
        ) * noiseStrength;

        p.velocity += noiseVec * deltaTime;
        p.position += p.velocity * deltaTime;

        if (p.type == 0) {
            float maxHeight = pointLight.position.y + 1.0f;
            if (p.position.y > maxHeight) {
                p.position.y = maxHeight;
                p.velocity.y *= 0.3f;
            }
        }
        else if (p.type == 1) {
            p.velocity.y += 0.03f * deltaTime;
        }
        else {
            p.velocity.y -= 0.5f * deltaTime;
        }

        glm::vec4 cStage = multiStageColor(p);

        if (p.type == 0) {
            float flicker = 0.8f + 0.05f * glm::linearRand(0.0f, 1.0f);
            cStage.a *= flicker;

            float heightAbove = p.position.y - pointLight.position.y;
            if (heightAbove > 0.3f) {
                float fadeFactor = glm::clamp(1.0f - (heightAbove - 0.3f) * 2.0f,
                    0.0f, 1.0f);
                cStage.a *= fadeFactor;
            }

            cStage.r *= p.color.r;
            cStage.g *= p.color.g;

            cStage.r = glm::clamp(cStage.r, 0.0f, 1.0f);
            cStage.g = glm::clamp(cStage.g, 0.0f, 1.0f);
            cStage.b = glm::clamp(cStage.b, 0.0f, 1.0f);
            cStage.a = glm::clamp(cStage.a, 0.0f, 1.0f);
        }
        p.color = cStage;

        if (p.type == 0) {
            if (lifeRatio > 0.9f) {
                float t = (1.0f - lifeRatio) / 0.1f;
                float maxSize = 0.22f;
                p.size = maxSize * t;
            }
            else {
                float t = lifeRatio / 0.9f;
                p.size = glm::mix(0.05f, 0.22f, t);
            }
        }
        else if (p.type == 1) {
            float startSize = 0.15f, endSize = 0.35f;
            p.size = glm::mix(endSize, startSize, lifeRatio);
        }
        else {
            p.size = glm::mix(0.02f, 0.1f, lifeRatio);
        }

        p.rotation += p.rotationSpeed * deltaTime;
        if (p.rotation > 360.f) p.rotation -= 360.f;
        if (p.rotation < 0.f)   p.rotation += 360.f;

        if (p.type == 2 && glm::linearRand(0.0f, 1.0f) < 0.05f && fireParticles.size() < MAX_TOTAL_PARTICLES) {
            FireParticle trail;
            trail.position = p.position - p.velocity * 0.02f;
            trail.velocity = glm::vec3(0.0f);
            trail.life = 0.5f;
            trail.initialLife = 0.5f;
            trail.color = glm::vec4(1.0f);
            trail.size = 0.03f;
            trail.rotation = glm::linearRand(0.0f, 360.0f);
            trail.rotationSpeed = glm::linearRand(-80.0f, 80.0f);
            trail.type = 2;
            trail.textureIndex = glm::linearRand(0, 6);
            fireParticles.push_back(trail);
        }


    }

    glBindBuffer(GL_ARRAY_BUFFER, instanceVBO);
    glBufferSubData(GL_ARRAY_BUFFER, 0,
        fireParticles.size() * sizeof(FireParticle),
        &fireParticles[0]);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
}


//renderers

void renderRain() {
    if (!rainEnabled) return;

    rainShader.useShaderProgram();
    glUniformMatrix4fv(rainUniforms.view, 1, GL_FALSE, glm::value_ptr(view));
    glUniformMatrix4fv(rainUniforms.projection, 1, GL_FALSE, glm::value_ptr(projection));
    glUniform3fv(rainUniforms.viewPosition, 1, glm::value_ptr(myCamera.getPosition()));

    glUniform3fv(rainUniforms.spotLight.position, 1, glm::value_ptr(spotLight.position));
    glUniform3fv(rainUniforms.spotLight.direction, 1, glm::value_ptr(spotLight.direction));

    glUniform1i(rainUniforms.dirLight.enabled, dirLight.enabled);
    glUniform1i(rainUniforms.spotLight.enabled, spotLight.enabled);
    glUniform1i(rainUniforms.flashLight.enabled, flashLight.enabled);
    glUniform1i(rainUniforms.pointLight.enabled, pointLight.enabled);
    glUniform1i(rainUniforms.leftHeadlight.enabled, leftHeadlight.enabled);
    glUniform1i(rainUniforms.rightHeadlight.enabled, rightHeadlight.enabled);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_CUBE_MAP, isDay ? daySkybox->getCubemapTexture() : nightSkybox->getCubemapTexture());
    glUniform1i(rainUniforms.environmentMap, 0);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    glBindVertexArray(rainVAO);
    glDrawArrays(GL_POINTS, 0, NUM_RAINDROPS);
    glBindVertexArray(0);

    glDisable(GL_BLEND);
}


void renderForest(gps::Shader& shader, const glm::mat4& viewMatrix, const glm::mat4& projectionMatrix) {

    shader.useShaderProgram();

    glm::mat4 model = glm::rotate(glm::mat4(1.0f), glm::radians(angle), glm::vec3(0.0f, 1.0f, 0.0f));

    if (shader.shaderProgram == myBasicShader.shaderProgram) {
        glUniformMatrix4fv(basicUniforms.model, 1, GL_FALSE, glm::value_ptr(model));
    }
    else if (shader.shaderProgram == shadowShader.shaderProgram) {
        glUniformMatrix4fv(shadowUniforms.model, 1, GL_FALSE, glm::value_ptr(model));
    }
    else if (shader.shaderProgram == pointShadowShader.shaderProgram) {
        glUniformMatrix4fv(pointShadowUniforms.model, 1, GL_FALSE, glm::value_ptr(model));
    }
    else if (shader.shaderProgram == headShadowShader.shaderProgram) {
        glUniformMatrix4fv(headShadowUniforms.model, 1, GL_FALSE, glm::value_ptr(model));
    }
    else {
        std::cerr << "Warning: Model uniform location not defined for the current shader program!" << std::endl;
    }

    // Draw the forest with frustum culling
    forest.Draw(shader, viewMatrix, projectionMatrix);
}


void renderDepthMap() {
    glm::vec3 lightPos = glm::normalize(-dirLight.direction) * 180.0f;

    lightProjection = glm::ortho(-75.0f, 75.0f, -75.0f, 75.0f, 0.1f, 300.0f);
    lightView = glm::lookAt(
        lightPos,
        glm::vec3(0.0f, 0.0f, 0.0f),
        glm::vec3(0.0f, 1.0f, 0.0f)
    );
    lightSpaceMatrix = lightProjection * lightView;

    shadowShader.useShaderProgram();
    glUniformMatrix4fv(shadowUniforms.lightSpaceMatrix, 1, GL_FALSE, glm::value_ptr(lightSpaceMatrix));
    glUniform1f(shadowUniforms.u_Time, u_Time);
    glUniform3fv(shadowUniforms.u_WindDirection, 1, glm::value_ptr(u_WindDirection));
    glUniform1f(shadowUniforms.u_WindStrength, u_WindStrength);
	glUniform1f(shadowUniforms.u_GustSize, gustSize);
	glUniform1f(shadowUniforms.u_GustSpeed, gustSpeed);
	glUniform1f(shadowUniforms.u_WindWaveLength, windWaveLength);

    glUniform1i(shadowUniforms.windEnabled, windEnabled);

    glEnable(GL_DEPTH_TEST);
    glViewport(0, 0, SHADOW_WIDTH, SHADOW_HEIGHT);
    glBindFramebuffer(GL_FRAMEBUFFER, shadowMapFBO);

    glClear(GL_DEPTH_BUFFER_BIT);
	renderForest(shadowShader, lightView, lightProjection);

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void renderDepthCubemap() {
    float nearPlane = 0.1f;
    float farPlane = 300.0f;
    glm::mat4 shadowProj = glm::perspective(glm::radians(90.0f), 1.0f, nearPlane, farPlane);

    std::vector<glm::mat4> shadowTransforms = {
        shadowProj * glm::lookAt(pointLight.position, pointLight.position + glm::vec3(1.0f, 0.0f, 0.0f), glm::vec3(0.0f, -1.0f, 0.0f)),
        shadowProj * glm::lookAt(pointLight.position, pointLight.position + glm::vec3(-1.0f, 0.0f, 0.0f), glm::vec3(0.0f, -1.0f, 0.0f)),
        shadowProj * glm::lookAt(pointLight.position, pointLight.position + glm::vec3(0.0f, 1.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f)),
        shadowProj * glm::lookAt(pointLight.position, pointLight.position + glm::vec3(0.0f, -1.0f, 0.0f), glm::vec3(0.0f, 0.0f, -1.0f)),
        shadowProj * glm::lookAt(pointLight.position, pointLight.position + glm::vec3(0.0f, 0.0f, 1.0f), glm::vec3(0.0f, -1.0f, 0.0f)),
        shadowProj * glm::lookAt(pointLight.position, pointLight.position + glm::vec3(0.0f, 0.0f, -1.0f), glm::vec3(0.0f, -1.0f, 0.0f))
    };

    pointShadowShader.useShaderProgram();
    for (unsigned int i = 0; i < 6; ++i) {
        glUniformMatrix4fv(pointShadowUniforms.shadowMatrices[i], 1, GL_FALSE, glm::value_ptr(shadowTransforms[i]));
    }

    glUniform1f(pointShadowUniforms.far_plane, farPlane);
    glUniform3fv(pointShadowUniforms.lightPos, 1, glm::value_ptr(pointLight.position));
    glUniform1f(pointShadowUniforms.u_Time, u_Time);
    glUniform3fv(pointShadowUniforms.u_WindDirection, 1, glm::value_ptr(u_WindDirection));
    glUniform1f(pointShadowUniforms.u_WindStrength, u_WindStrength);
	glUniform1f(pointShadowUniforms.u_GustSize, gustSize);
	glUniform1f(pointShadowUniforms.u_GustSpeed, gustSpeed);
	glUniform1f(pointShadowUniforms.u_WindWaveLength, windWaveLength);
    glUniform1i(pointShadowUniforms.windEnabled, windEnabled);


    glViewport(0, 0, POINT_SHADOW_WIDTH, POINT_SHADOW_HEIGHT);
    glBindFramebuffer(GL_FRAMEBUFFER, pointLightFBO);
    glClear(GL_DEPTH_BUFFER_BIT);

	renderForest(pointShadowShader, shadowTransforms[0], shadowProj);

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}


glm::mat4 renderHeadlightDepthMap(SpotLight& headlight, GLuint FBO, GLuint depthMap) {
    glm::mat4 lightProjectionHead = glm::perspective(glm::radians(60.0f), 1.0f, 0.1f, 300.0f);
    glm::mat4 lightViewHead = glm::lookAt(
        headlight.position,
        headlight.position + headlight.direction,
        glm::vec3(0.0f, 1.0f, 0.0f)
    );
    glm::mat4 lightSpaceMatrixHead = lightProjectionHead * lightViewHead;

    headShadowShader.useShaderProgram();
    glUniformMatrix4fv(headShadowUniforms.lightSpaceMatrixHead, 1, GL_FALSE, glm::value_ptr(lightSpaceMatrixHead));
    glUniform1f(headShadowUniforms.u_Time, u_Time);
    glUniform3fv(headShadowUniforms.u_WindDirection, 1, glm::value_ptr(u_WindDirection));
    glUniform1f(headShadowUniforms.u_WindStrength, u_WindStrength);\
	glUniform1f(headShadowUniforms.u_GustSize, gustSize);
	glUniform1f(headShadowUniforms.u_GustSpeed, gustSpeed);
	glUniform1f(headShadowUniforms.u_WindWaveLength, windWaveLength);
    glUniform1i(headShadowUniforms.windEnabled, windEnabled);


    glViewport(0, 0, SPOT_LIGHT_SHADOW_WIDTH, SPOT_LIGHT_SHADOW_HEIGHT);
    glBindFramebuffer(GL_FRAMEBUFFER, FBO);
    glClear(GL_DEPTH_BUFFER_BIT);

	renderForest(headShadowShader, lightViewHead, lightProjectionHead);

    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    return lightSpaceMatrixHead;
}


void renderFire(float currentTime)
{
    fireShader.useShaderProgram();

	glUniformMatrix4fv(fireUniforms.view, 1, GL_FALSE, glm::value_ptr(view));
    glUniformMatrix4fv(fireUniforms.projection, 1, GL_FALSE, glm::value_ptr(projection));

    glUniform3fv(fireUniforms.cameraRight, 1, glm::value_ptr(myCamera.getRight()));
    glUniform3fv(fireUniforms.cameraUp, 1, glm::value_ptr(myCamera.getUp()));
    glUniform1f(fireUniforms.time, currentTime);
    glUniform1f(fireUniforms.flameAspectX, 1.0f);
    glUniform1f(fireUniforms.flameAspectY, 3.0f);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D_ARRAY, fireTextureArray);
	glUniform1i(fireUniforms.fireTextureArray, 0);
    glDepthMask(GL_FALSE);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE);

    glBindVertexArray(quad2VAO);

    std::vector<FireParticle> pass1;
    pass1.reserve(fireParticles.size());
    for (auto& p : fireParticles) {
        if (p.type == 0 || p.type == 2) {
            pass1.push_back(p);
        }
    }
    glBindBuffer(GL_ARRAY_BUFFER, instanceVBO);
    glBufferSubData(GL_ARRAY_BUFFER, 0, pass1.size() * sizeof(FireParticle), &pass1[0]);
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    glDrawArraysInstanced(GL_TRIANGLES, 0, 6, pass1.size());

    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    std::vector<FireParticle> pass2;
    pass2.reserve(fireParticles.size());
    for (auto& p : fireParticles) {
        if (p.type == 1) {
            pass2.push_back(p);
        }
    }
    glBindBuffer(GL_ARRAY_BUFFER, instanceVBO);
    glBufferSubData(GL_ARRAY_BUFFER, 0, pass2.size() * sizeof(FireParticle), &pass2[0]);
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    glDrawArraysInstanced(GL_TRIANGLES, 0, 6, pass2.size());

    glBindVertexArray(0);
    glDepthMask(GL_TRUE);
    glDisable(GL_BLEND);

	myBasicShader.useShaderProgram();

}

void renderQuad()
{
    if (quadVAO == 0)
    {
        float quadVertices[] = {
            -1.0f,  1.0f, 0.0f, 0.0f, 1.0f,
            -1.0f, -1.0f, 0.0f, 0.0f, 0.0f,
             1.0f,  1.0f, 0.0f, 1.0f, 1.0f,
             1.0f, -1.0f, 0.0f, 1.0f, 0.0f,
        };
        glGenVertexArrays(1, &quadVAO);
        glGenBuffers(1, &quadVBO);
        glBindVertexArray(quadVAO);
        glBindBuffer(GL_ARRAY_BUFFER, quadVBO);
        glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), &quadVertices, GL_STATIC_DRAW);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));
    }
    glBindVertexArray(quadVAO);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    glBindVertexArray(0);
}

void renderScene() {

    spotLight.position = myCamera.getPosition();
    spotLight.direction = myCamera.getFront();

    static double lastFrameTime = glfwGetTime();
    double currentFrameTime = glfwGetTime();
    float deltaTime = static_cast<float>(currentFrameTime - lastFrameTime);
    lastFrameTime = currentFrameTime;

    fogTime += 0.00075f * (static_cast<float>(rand()) / 10.0f) * deltaTime;



    float currentTime = static_cast<float>(glfwGetTime());
    u_Time = currentTime;

	float windAngle = u_Time * 0.15f;
    u_WindDirection = glm::normalize(glm::vec3(
        cos(windAngle),
        0.0f,
        sin(windAngle)
    ));

    if (pointLight.enabled) {
        updatePointLightIntensity(pointLight, deltaTime);

        myBasicShader.useShaderProgram();
        glUniform3fv(basicUniforms.pointLight.ambient, 1, glm::value_ptr(pointLight.ambient));
        glUniform3fv(basicUniforms.pointLight.diffuse, 1, glm::value_ptr(pointLight.diffuse));
        glUniform3fv(basicUniforms.pointLight.specular, 1, glm::value_ptr(pointLight.specular));
        glUniform1f(basicUniforms.pointLight.constant, pointLight.constant);
        glUniform1f(basicUniforms.pointLight.linear, pointLight.linear);
        glUniform1f(basicUniforms.pointLight.quadratic, pointLight.quadratic);
    }

    renderDepthMap();
    renderDepthCubemap();
    glm::mat4 leftHeadlightLightSpaceMatrix = renderHeadlightDepthMap(leftHeadlight, leftHeadlightFBO, leftHeadlightDepthMap);
    glm::mat4 rightHeadlightLightSpaceMatrix = renderHeadlightDepthMap(rightHeadlight, rightHeadlightFBO, rightHeadlightDepthMap);

    glGetIntegerv(GL_POLYGON_MODE, polygonModeParams);
    currentPolygonMode = polygonModeParams[0];

    isWireframe = (currentPolygonMode == GL_LINE);
    isPointMode = (currentPolygonMode == GL_POINT);
    if (isPointMode) {
        glPointSize(5.0f);
    }
    else {
        glPointSize(1.0f);
    }

	currentTime = static_cast<float>(glfwGetTime());
    if (transitioning) {
        float elapsed = currentTime - transitionStartTime;
        float progress = elapsed / transitionDuration;
        if (progress >= 1.0f) {
            progress = 1.0f;
            transitioning = false;
        }
        blend = startBlend + (targetBlend - startBlend) * progress;
    }

    dirLight.direction = glm::mix(nightDirection, dayDirection, blend);
    dirLight.ambient = glm::mix(nightAmbient, dayAmbient, blend);
    dirLight.diffuse = glm::mix(nightDiffuse, dayDiffuse, blend);
    dirLight.specular = glm::mix(nightSpecular, daySpecular, blend);
    dirLight.color = glm::mix(nightColor, dayColor, blend);

    lakeLightPosition = glm::mix(lakeNightPosition, lakeDayPosition, blend);
    lakeLightColor = glm::mix(lakeNightColor, lakeDayColor, blend);

    glm::vec3 currentFogColor = glm::mix(nightFogColor, dayFogColor, blend);
	float currentgFogEnd = glm::mix(nightgFogEnd, daygFogEnd, blend);
	float currentgExpFogDensity = glm::mix(nightgExpFogDensity, daygExpFogDensity, blend);
	float currentgLayeredFogTop = glm::mix(nightgLayeredFogTop, daygLayeredFogTop, blend);

    myBasicShader.useShaderProgram();
    glUniform1f(basicUniforms.u_Time, u_Time);
    glUniform3fv(basicUniforms.u_WindDirection, 1, glm::value_ptr(u_WindDirection));
    glUniform1f(basicUniforms.u_WindStrength, u_WindStrength);

    glUniform1i(basicUniforms.dirLight.enabled, dirLight.enabled);
    glUniform1i(basicUniforms.pointLight.enabled, pointLight.enabled);
    glUniform1i(basicUniforms.spotLight.enabled, spotLight.enabled);
    glUniform1i(basicUniforms.leftHeadlight.enabled, leftHeadlight.enabled);
    glUniform1i(basicUniforms.rightHeadlight.enabled, rightHeadlight.enabled);

    glUniform3fv(basicUniforms.dirLight.direction, 1, glm::value_ptr(dirLight.direction));
    glUniform3fv(basicUniforms.dirLight.ambient, 1, glm::value_ptr(dirLight.ambient));
    glUniform3fv(basicUniforms.dirLight.diffuse, 1, glm::value_ptr(dirLight.diffuse));
    glUniform3fv(basicUniforms.dirLight.specular, 1, glm::value_ptr(dirLight.specular));
    glUniform3fv(basicUniforms.dirLight.color, 1, glm::value_ptr(dirLight.color));

    glUniform3fv(basicUniforms.viewPosition, 1, glm::value_ptr(myCamera.getPosition()));

    glUniform3fv(basicUniforms.spotLight.position, 1, glm::value_ptr(spotLight.position));
    glUniform3fv(basicUniforms.spotLight.direction, 1, glm::value_ptr(spotLight.direction));

    glUniform1i(basicUniforms.useNormalMapping, useNormalMapping);
    glUniform1i(basicUniforms.rainEnabled, rainEnabled);
    glUniform1i(basicUniforms.windEnabled, windEnabled);
    glUniform1i(basicUniforms.fogEnabled, fogEnabled);

	glUniform3fv(basicUniforms.fogColor, 1, glm::value_ptr(currentFogColor));
	glUniform1f(basicUniforms.gFogEnd, currentgFogEnd);
	glUniform1f(basicUniforms.gExpFogDensity, currentgExpFogDensity);
	glUniform1f(basicUniforms.gLayeredFogTop, currentgLayeredFogTop);
	glUniform1f(basicUniforms.gFogTime, fogTime);


    glUniformMatrix4fv(basicUniforms.lightSpaceMatrix, 1, GL_FALSE, glm::value_ptr(lightSpaceMatrix));
    glUniform1f(basicUniforms.farPlane, 300.0f);

    glUniformMatrix4fv(basicUniforms.leftHeadlightLightSpaceMatrix, 1, GL_FALSE, glm::value_ptr(leftHeadlightLightSpaceMatrix));
    glUniformMatrix4fv(basicUniforms.rightHeadlightLightSpaceMatrix, 1, GL_FALSE, glm::value_ptr(rightHeadlightLightSpaceMatrix));

    glActiveTexture(GL_TEXTURE0 + 4);
    glBindTexture(GL_TEXTURE_2D, depthMap);
    glUniform1i(basicUniforms.shadowMap, 4);

    glActiveTexture(GL_TEXTURE0 + 5);
    glBindTexture(GL_TEXTURE_CUBE_MAP, depthCubemap);
    glUniform1i(basicUniforms.pointLightShadowMap, 5);

    glActiveTexture(GL_TEXTURE0 + 6);
    glBindTexture(GL_TEXTURE_2D, leftHeadlightDepthMap);
    glUniform1i(basicUniforms.leftHeadlightShadowMap, 6);

    glActiveTexture(GL_TEXTURE0 + 7);
    glBindTexture(GL_TEXTURE_2D, rightHeadlightDepthMap);
    glUniform1i(basicUniforms.rightHeadlightShadowMap, 7);

    skyboxShader.useShaderProgram();

    glm::mat4 skyboxView = glm::mat4(glm::mat3(view));

    glUniformMatrix4fv(skyboxUniforms.view, 1, GL_FALSE, glm::value_ptr(skyboxView));
    glUniformMatrix4fv(skyboxUniforms.projection, 1, GL_FALSE, glm::value_ptr(projection));

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_CUBE_MAP, daySkybox->getCubemapTexture());
    glUniform1i(skyboxUniforms.daySkybox, 0);

    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_CUBE_MAP, nightSkybox->getCubemapTexture());
    glUniform1i(skyboxUniforms.nightSkybox, 1);

    glUniform1f(skyboxUniforms.blendFactor, blend);


    waterFrameBuffers->bindReflectionFrameBuffer();
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    float distance = 2 * (myCamera.getPosition().y - waterTiles[0].getHeight());
    glm::vec3 position = myCamera.getPosition();
    position.y -= distance;
    myCamera.setPosition(position);
    myCamera.invertPitch();
    view = myCamera.getViewMatrix();
    myBasicShader.useShaderProgram();
    glUniformMatrix4fv(basicUniforms.view, 1, GL_FALSE, glm::value_ptr(view));
    glUniform4fv(basicUniforms.clipPlane, 1, glm::value_ptr(glm::vec4(0, 1, 0, -waterTiles[0].getHeight() + 1.0f)));
    daySkybox->Draw(skyboxShader);

	renderForest(myBasicShader, view, projection);

    if (pointLight.enabled) {
        renderFire(currentTime);
    }
    if (rainEnabled) {
        renderRain();
    }
    position.y += distance;
    myCamera.setPosition(position);
    myCamera.invertPitch();
    view = myCamera.getViewMatrix();
    waterFrameBuffers->bindRefractionFrameBuffer();
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    myBasicShader.useShaderProgram();
    glUniformMatrix4fv(basicUniforms.view, 1, GL_FALSE, glm::value_ptr(view));
    glUniform4fv(basicUniforms.clipPlane, 1, glm::value_ptr(glm::vec4(0, -1, 0, waterTiles[0].getHeight())));
    daySkybox->Draw(skyboxShader);
	renderForest(myBasicShader, view, projection);
    if (pointLight.enabled) {
        renderFire(currentTime);
    }
    if (rainEnabled) {
        renderRain();
    }

    waterFrameBuffers->unbindCurrentFrameBuffer(myWindow.getWindowDimensions().width, myWindow.getWindowDimensions().height);
    performHDR = hdrEnabled && !isWireframe && !isPointMode;
    if (performHDR) {
        glBindFramebuffer(GL_FRAMEBUFFER, hdrFBO);
    }
    else {
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
    }

    glViewport(0, 0, myWindow.getWindowDimensions().width, myWindow.getWindowDimensions().height);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    myBasicShader.useShaderProgram();
    glUniform4fv(basicUniforms.clipPlane, 1, glm::value_ptr(glm::vec4(0, 1, 0, 10000)));
    daySkybox->Draw(skyboxShader);

	renderForest(myBasicShader, view, projection);

    if (pointLight.enabled) {
        renderFire(currentTime);
    }
    if (rainEnabled) {
        renderRain();
    }

    GLuint reflectionTexture = waterFrameBuffers->getReflectionTexture();
    GLuint refractionTexture = waterFrameBuffers->getRefractionTexture();
    GLuint depthTexture = waterFrameBuffers->getRefractionDepthTexture();

    currentTime = glfwGetTime();
    waterRenderer->render(waterTiles, view, myCamera.getPosition(), reflectionTexture, refractionTexture, depthTexture, deltaTime, lakeLightPosition, lakeLightColor);
    if (performHDR) {
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
    }


}

//blur extractor for bloom
void blurBrightTexture()
{
    bool horizontal = true, firstIteration = true;
    blurShader.useShaderProgram();
    unsigned int amount = blurIterations;

    for (unsigned int i = 0; i < amount; i++)
    {
        glBindFramebuffer(GL_FRAMEBUFFER, pingpongFBO[horizontal]);
        glUniform1i(blurUniforms.horizontal, horizontal);

        glActiveTexture(GL_TEXTURE0);
        if (firstIteration) {
            glBindTexture(GL_TEXTURE_2D, colorBuffers[1]);
        }
        else {
            glBindTexture(GL_TEXTURE_2D, pingpongColorbuffers[!horizontal]);
        }
        renderQuad();

        horizontal = !horizontal;
        if (firstIteration)
            firstIteration = false;
    }
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}


//resizers,movement and callbacks
void toggleFullscreen(bool enable) {
    if (enable) {
        const GLFWvidmode* mode = glfwGetVideoMode(glfwGetPrimaryMonitor());
        glfwSetWindowMonitor(myWindow.getWindow(), glfwGetPrimaryMonitor(), 0, 0, mode->width, mode->height, mode->refreshRate);
        myWindow.setWindowDimensions({ mode->width, mode->height });
    }
    else {
        glfwSetWindowMonitor(myWindow.getWindow(), NULL, 100, 100, 1024, 768, 0);
        myWindow.setWindowDimensions({ 1024, 768 });
    }
}

void keyboardCallback(GLFWwindow* window, int key, int scancode, int action, int mode) {
    AppState* appState = static_cast<AppState*>(glfwGetWindowUserPointer(window));
    if (!appState || !appState->audioManager) return;

    AudioManager* audioMgr = appState->audioManager;
    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS) {
        glfwSetWindowShouldClose(window, GL_TRUE);
    }

    if (key >= 0 && key < 1024) {
        if (action == GLFW_PRESS) {
            pressedKeys[key] = true;
        }
        else if (action == GLFW_RELEASE) {
            pressedKeys[key] = false;
        }
    }

    if (key == GLFW_KEY_B && action == GLFW_PRESS && !bloomKeyPressed) {
        bloomEnabled = !bloomEnabled;
        bloomKeyPressed = true;
        std::cout << "Bloom Toggled: " << (bloomEnabled ? "ON" : "OFF") << std::endl;
    }
    if (key == GLFW_KEY_B && action == GLFW_RELEASE) {
        bloomKeyPressed = false;
    }



    if (pressedKeys[GLFW_KEY_1]) {
        glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
        glPointSize(1.0f);
    }
    else if (pressedKeys[GLFW_KEY_2]) {
        glPolygonMode(GL_FRONT_AND_BACK, GL_POINT);
        glPointSize(5.0f);
    }
    else {
        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
        glPointSize(1.0f);
    }

    if (key == GLFW_KEY_T) {
        isTourActive = true;
        currentWaypoint = 0;
        t = 0.0f;
        std::cout << "Tour started" << std::endl;
    }
    if (key == GLFW_KEY_Y) {
        isTourActive = false;
        std::cout << "Tour stopped manually" << std::endl;
    }

    if (key == GLFW_KEY_N && action == GLFW_PRESS) {
        useNormalMapping = !useNormalMapping;
        std::cout << "Normal Mapping Toggled: " << (useNormalMapping ? "ON" : "OFF") << std::endl;
    }

    if (key == GLFW_KEY_3 && action == GLFW_PRESS) {
        dirLight.enabled = !dirLight.enabled;
        std::cout << "Directional Light Toggled: " << (dirLight.enabled ? "ON" : "OFF") << std::endl;
    }
    if (key == GLFW_KEY_4 && action == GLFW_PRESS) {
        pointLight.enabled = !pointLight.enabled;
        std::cout << "Point Light Toggled: " << (pointLight.enabled ? "ON" : "OFF") << std::endl;
        if (pointLight.enabled) {
            audioMgr->playFire(true);
            firePlaying = true;
        }
        else {
            audioMgr->stopFire();
            firePlaying = false;
        }
    }
    if (key == GLFW_KEY_5 && action == GLFW_PRESS) {
        spotLight.enabled = !spotLight.enabled;
        std::cout << "Spot Light Toggled: " << (spotLight.enabled ? "ON" : "OFF") << std::endl;
    }

    if (key == GLFW_KEY_6 && action == GLFW_PRESS) {
        leftHeadlight.enabled = !leftHeadlight.enabled;
        rightHeadlight.enabled = !rightHeadlight.enabled;
    }

    if (key == GLFW_KEY_UP && action == GLFW_PRESS) {
        if (isDay && !transitioning) {
            dayDirection.y += 0.1f;
        }
        else if (!isDay && !transitioning) {
            nightDirection.y += 0.1f;
        }
    }

    if (key == GLFW_KEY_DOWN && action == GLFW_PRESS) {
        if (isDay && !transitioning) {
            dayDirection.y -= 0.1f;
        }
        else if (!isDay && !transitioning) {
            nightDirection.y -= 0.1f;
        }
    }

    if (key == GLFW_KEY_LEFT && action == GLFW_PRESS) {
        if (isDay && !transitioning) {
            dayDirection.x -= 0.1f;
        }
        else if (!isDay && !transitioning) {
            nightDirection.x -= 0.1f;
        }
    }

    if (key == GLFW_KEY_RIGHT && action == GLFW_PRESS) {
        if (isDay && !transitioning) {
            dayDirection.x += 0.1f;
        }
        else if (!isDay && !transitioning) {
            nightDirection.x += 0.1f;
        }
    }

    if (key == GLFW_KEY_PAGE_UP && action == GLFW_PRESS) {
        if (isDay && !transitioning) {
            dayDirection.z += 0.1f;
        }
        else if (!isDay && !transitioning) {
            nightDirection.z += 0.1f;
        }
    }

    if (key == GLFW_KEY_PAGE_DOWN && action == GLFW_PRESS) {
        if (isDay && !transitioning) {
            dayDirection.z -= 0.1f;
        }
        else if (!isDay && !transitioning) {
            nightDirection.z -= 0.1f;
        }
    }

    if (key == GLFW_KEY_F11 && action == GLFW_PRESS) {
        static bool isFullscreen = false;
        isFullscreen = !isFullscreen;
        toggleFullscreen(isFullscreen);
    }

    if (key == GLFW_KEY_F && action == GLFW_PRESS) {
        fogEnabled = !fogEnabled;
        std::cout << "Fog Toggled: " << (fogEnabled ? "ON" : "OFF") << std::endl;
    }

    if (key == GLFW_KEY_H && action == GLFW_PRESS && !transitioning) {
        isDay = !isDay;
        transitioning = true;
        transitionStartTime = glfwGetTime();
        startBlend = blend;
        targetBlend = isDay ? 1.0f : 0.0f;
        std::cout << "Toggling Day/Night: " << (isDay ? "DAY" : "NIGHT") << std::endl;
    }

    if (key == GLFW_KEY_R && action == GLFW_PRESS) {
        rainEnabled = !rainEnabled;
        std::cout << "Rain Toggled: " << (rainEnabled ? "ON" : "OFF") << std::endl;
        if (rainEnabled && !rainPlaying) {
            audioMgr->playRain(true);
            rainPlaying = true;
        }
        else if (!rainEnabled && rainPlaying) {
            audioMgr->stopRain();
            rainPlaying = false;

            if (isFlashing) {
                isFlashing = false;
                flashLight.enabled = false;
                globalLightIntensity = 1.0f;
                myBasicShader.useShaderProgram();
                glUniform1i(basicUniforms.flashLight.enabled, flashLight.enabled);
                glUniform1f(basicUniforms.globalLightIntensity, globalLightIntensity);
                std::cout << "Ongoing flash terminated due to rain being disabled." << std::endl;
            }
        }


    }

    if (key == GLFW_KEY_F2 && action == GLFW_PRESS) {
        audioMgr->increaseRainVolume();
        audioMgr->increaseThunderVolume();
        audioMgr->increaseFireVolume();
    }
    if (key == GLFW_KEY_F3 && action == GLFW_PRESS) {
        audioMgr->decreaseRainVolume();
        audioMgr->decreaseThunderVolume();
        audioMgr->decreaseFireVolume();
    }

    if (key == GLFW_KEY_7 && action == GLFW_PRESS) {
        windEnabled = !windEnabled;
        std::cout << "Wind Toggled: " << (windEnabled ? "ON" : "OFF") << std::endl;
    }

    if (pressedKeys[GLFW_KEY_F5]) {
        exposure += 0.1f;
        std::cout << "Exposure: " << exposure << std::endl;
    }
    if (pressedKeys[GLFW_KEY_F6]) {
        exposure -= 0.1f;
        std::cout << "Exposure: " << exposure << std::endl;
    }

    if (key == GLFW_KEY_F4 && action == GLFW_PRESS) {
        hdrEnabled = !hdrEnabled;
        std::cout << "HDR Toggled: " << (hdrEnabled ? "ON" : "OFF") << std::endl;

        if (hdrEnabled) {
            glDisable(GL_FRAMEBUFFER_SRGB);
        }
        else {
            glEnable(GL_FRAMEBUFFER_SRGB);
        }

    }

    if (key == GLFW_KEY_P && action == GLFW_PRESS && !recordingKeyPressed) {
        recordWaypoint();
        recordingKeyPressed = true;
    }
    if (key == GLFW_KEY_P && action == GLFW_RELEASE) {
        recordingKeyPressed = false;
    }

    if (key == GLFW_KEY_O && action == GLFW_PRESS && !saveKeyPressed) {
        saveWaypoints("waypoints.txt");
        saveKeyPressed = true;
    }
    if (key == GLFW_KEY_O && action == GLFW_RELEASE) {
        saveKeyPressed = false;
    }

    if (key == GLFW_KEY_L && action == GLFW_PRESS && !loadKeyPressed) {
        loadWaypoints("waypoints.txt");
        loadKeyPressed = true;
    }
    if (key == GLFW_KEY_L && action == GLFW_RELEASE) {
        loadKeyPressed = false;
    }

    if (key == GLFW_KEY_MINUS && action == GLFW_PRESS && !removeKeyPressed) {
        if (!keyLocations.empty()) {
            keyLocations.pop_back();
            std::cout << "Last Waypoint Removed. Total Waypoints: " << keyLocations.size() << std::endl;
        }
        else {
        }
        removeKeyPressed = true;
    }
    if (key == GLFW_KEY_MINUS && action == GLFW_RELEASE) {
        removeKeyPressed = false;
    }

}

void mouseCallback(GLFWwindow* window, double xpos, double ypos) {
    if (isTourActive) return;
    myCamera.updateMousePosition(xpos, ypos);
}

void resizeHDRFramebuffer(int width, int height) {
    glDeleteTextures(2, colorBuffers);

    for (unsigned int i = 0; i < 2; i++) {
        glGenTextures(1, &colorBuffers[i]);
        glBindTexture(GL_TEXTURE_2D, colorBuffers[i]);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, width, height, 0, GL_RGBA, GL_FLOAT, NULL);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

        glBindFramebuffer(GL_FRAMEBUFFER, hdrFBO);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + i, GL_TEXTURE_2D, colorBuffers[i], 0);
    }

    glDeleteRenderbuffers(1, &rbo);

    glGenRenderbuffers(1, &rbo);
    glBindRenderbuffer(GL_RENDERBUFFER, rbo);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT, width, height);

    glBindFramebuffer(GL_FRAMEBUFFER, hdrFBO);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, rbo);

    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
        std::cout << "Resized HDR Framebuffer not complete!" << std::endl;

    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    glDeleteTextures(2, pingpongColorbuffers);

    for (unsigned int i = 0; i < 2; i++) {
        glGenTextures(1, &pingpongColorbuffers[i]);
        glBindTexture(GL_TEXTURE_2D, pingpongColorbuffers[i]);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, width, height, 0, GL_RGBA, GL_FLOAT, NULL);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

        glBindFramebuffer(GL_FRAMEBUFFER, pingpongFBO[i]);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, pingpongColorbuffers[i], 0);

        if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
            std::cout << "Resized Ping-Pong FBO " << i << " not complete!" << std::endl;
    }

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void windowResizeCallback(GLFWwindow* window, int width, int height) {
    fprintf(stdout, "Window resized! New width: %d , height: %d\n", width, height);
    if (width == 0 || height == 0) {
        fprintf(stdout, "Ignored resize event with zero dimensions.\n");
        return;
    }
    glViewport(0, 0, width, height);

    AppState* appState = static_cast<AppState*>(glfwGetWindowUserPointer(window));
    if (appState && appState->window) {
        appState->window->setWindowDimensions({ width, height });
    }

    projection = glm::perspective(glm::radians(myCamera.getFov()),
        static_cast<float>(width) / static_cast<float>(height),
        0.1f, 300.0f);
    myBasicShader.useShaderProgram();
    glUniformMatrix4fv(basicUniforms.projection, 1, GL_FALSE, glm::value_ptr(projection));

    double xpos, ypos;
    glfwGetCursorPos(window, &xpos, &ypos);
    myCamera.setLastMousePosition(static_cast<float>(xpos), static_cast<float>(ypos));

    resizeHDRFramebuffer(width, height);

    if (waterFrameBuffers)
    {
        waterFrameBuffers->resize(width, height);
    }

}

void scroll_callback(GLFWwindow* window, double xoffset, double yoffset)
{
    myCamera.zoom(static_cast<float>(yoffset));

    int width, height;
    glfwGetWindowSize(window, &width, &height);
    projection = glm::perspective(glm::radians(myCamera.getFov()),
        static_cast<float>(width) / static_cast<float>(height),
        0.1f, 300.0f);

    myBasicShader.useShaderProgram();
    glUniformMatrix4fv(basicUniforms.projection, 1, GL_FALSE, glm::value_ptr(projection));

    rainShader.useShaderProgram();
    glUniformMatrix4fv(rainUniforms.projection, 1, GL_FALSE, glm::value_ptr(projection));

    skyboxShader.useShaderProgram();
    glUniformMatrix4fv(skyboxUniforms.projection, 1, GL_FALSE, glm::value_ptr(projection));

    float currentFOV = myCamera.getFov();
    updateSpotlightRange(currentFOV);
}

void setWindowCallbacks() {
    glfwSetWindowSizeCallback(myWindow.getWindow(), windowResizeCallback);
    glfwSetKeyCallback(myWindow.getWindow(), keyboardCallback);
    glfwSetCursorPosCallback(myWindow.getWindow(), mouseCallback);
    glfwSetScrollCallback(myWindow.getWindow(), scroll_callback);
    glfwSetInputMode(myWindow.getWindow(), GLFW_CURSOR, GLFW_CURSOR_DISABLED);
    if (glfwRawMouseMotionSupported()) {
        glfwSetInputMode(myWindow.getWindow(), GLFW_RAW_MOUSE_MOTION, GLFW_TRUE);
        std::cout << "Raw mouse motion enabled." << std::endl;
    }
    else {
        std::cout << "Raw mouse motion not supported." << std::endl;
    }
}

void processMovement() {
    if (isTourActive) return;

    if (pressedKeys[GLFW_KEY_W]) {
        myCamera.move(gps::MOVE_FORWARD, cameraSpeed);
    }
    if (pressedKeys[GLFW_KEY_S]) {
        myCamera.move(gps::MOVE_BACKWARD, cameraSpeed);
    }
    if (pressedKeys[GLFW_KEY_A]) {
        myCamera.move(gps::MOVE_LEFT, cameraSpeed);
    }
    if (pressedKeys[GLFW_KEY_D]) {
        myCamera.move(gps::MOVE_RIGHT, cameraSpeed);
    }
    if (pressedKeys[GLFW_KEY_Q]) {
        myCamera.move(gps::MOVE_DOWN, cameraSpeed);
    }
    if (pressedKeys[GLFW_KEY_E]) {
        myCamera.move(gps::MOVE_UP, cameraSpeed);
    }

    view = myCamera.getViewMatrix();
    myBasicShader.useShaderProgram();
    glUniformMatrix4fv(basicUniforms.view, 1, GL_FALSE, glm::value_ptr(view));
}

//cleanup
void cleanup() {
    glDeleteVertexArrays(1, &quadVAO);
    glDeleteVertexArrays(1, &quad2VAO);
    glDeleteVertexArrays(1, &rainVAO);

    glDeleteBuffers(1, &quadVBO);
    glDeleteBuffers(1, &quad2VBO);
    glDeleteBuffers(1, &instanceVBO);
    glDeleteBuffers(1, &rainVBO);
    glDeleteBuffers(1, &rainParamsVBO);

    glDeleteTextures(2, colorBuffers);
    glDeleteTextures(2, pingpongColorbuffers);
    glDeleteTextures(1, &depthMap);
    glDeleteTextures(1, &depthCubemap);
    glDeleteTextures(1, &leftHeadlightDepthMap);
    glDeleteTextures(1, &rightHeadlightDepthMap);
    glDeleteTextures(1, &fireTextureArray);

    glDeleteFramebuffers(1, &hdrFBO);
    glDeleteFramebuffers(2, pingpongFBO);
    glDeleteFramebuffers(1, &shadowMapFBO);
    glDeleteFramebuffers(1, &pointLightFBO);
    glDeleteFramebuffers(1, &leftHeadlightFBO);
    glDeleteFramebuffers(1, &rightHeadlightFBO);

    glDeleteRenderbuffers(1, &rbo);

    glDeleteProgram(myBasicShader.shaderProgram);
    glDeleteProgram(skyboxShader.shaderProgram);
    glDeleteProgram(shadowShader.shaderProgram);
    glDeleteProgram(pointShadowShader.shaderProgram);
    glDeleteProgram(headShadowShader.shaderProgram);
    glDeleteProgram(rainShader.shaderProgram);
    glDeleteProgram(hdrShader.shaderProgram);
    glDeleteProgram(fireShader.shaderProgram);
    glDeleteProgram(blurShader.shaderProgram);

    delete daySkybox;
    delete nightSkybox;
    delete waterRenderer;
	delete waterFrameBuffers;

    audioManager.shutdown();

    glfwTerminate();
}

//main
int main(int argc, const char* argv[]) {

    try {
        initOpenGLWindow();
    }
    catch (const std::exception& e) {
        std::cerr << e.what() << std::endl;
        return EXIT_FAILURE;
    }

    initOpenGLState();
    initModels();
    initShaders();
    initShadowMapping();
    initPointLightShadowMapping();
    initHeadlightShadowMapping(leftHeadlightFBO, leftHeadlightDepthMap);
    initHeadlightShadowMapping(rightHeadlightFBO, rightHeadlightDepthMap);
    initUniforms();
    initRain();
    initHDRFramebuffer();
    initBloomBuffers();
    initFire();
    setWindowCallbacks();

    int windowWidth = myWindow.getWindowDimensions().width;
    int windowHeight = myWindow.getWindowDimensions().height;

    int reflectionWidth = windowWidth / 2;
    int reflectionHeight = windowHeight / 2;
    int refractionWidth = windowWidth / 2;
    int refractionHeight = windowHeight / 2;

    waterFrameBuffers = new WaterFrameBuffers(reflectionWidth, reflectionHeight, refractionWidth, refractionHeight);

    if (!audioManager.initialize()) {
        std::cerr << "Failed to initialize AudioManager." << std::endl;
    }

    if (!audioManager.loadRainSound("sounds/rain.wav")) {
        std::cerr << "Failed to load rain sound." << std::endl;
    }

    if (!audioManager.loadThunderSound("sounds/thunder1.mp3")) {
        std::cerr << "Failed to load thunder sound." << std::endl;
    }

    if (!audioManager.loadFireSound("sounds/fire.mp3")) {
        std::cerr << "Failed to load fire sound." << std::endl;
    }

    audioManager.setFirePosition(glm::vec3(pointLight.position.x, pointLight.position.y, pointLight.position.z));
    AppState appState;
    appState.window = &myWindow;
    appState.audioManager = &audioManager;
    glfwSetWindowUserPointer(myWindow.getWindow(), &appState);


    double prevTime = 0.0;
    double currentTime = 0.0;
    double timeDiff;
    unsigned int counter = 0;

    bool isThunderScheduled = false;
    float thunderDelay = 1.0f;
    float thunderTimer = 0.0f;

    srand(static_cast<unsigned int>(time(nullptr)));

    nextFlashTime = static_cast<float>(rand()) / RAND_MAX * (flashIntervalMax - flashIntervalMin) + flashIntervalMin;

    waterRenderer = new WaterRenderer(
        "shaders/water.vert",
        "shaders/water.frag",
        projection,
        "models/forest/textures/waterDUDV.png",
        "models/forest/textures/waterNORMAL.png");


   waterTiles.emplace_back(15.0f, 15.0f, -3.0f);
   loadWaypoints("waypoints.txt");

    while (!glfwWindowShouldClose(myWindow.getWindow())) {
        currentTime = glfwGetTime();
        timeDiff = currentTime - prevTime;
        counter++;
        if (timeDiff >= 1.0 / 30.0) {
            std::string FPS = std::to_string((1.0 / timeDiff) * counter);
            std::string ms = std::to_string((timeDiff / counter) * 1000);
            std::string title = "OpenGL Forest | FPS: " + FPS + " | ms: " + ms;
            glfwSetWindowTitle(myWindow.getWindow(), title.c_str());
            counter = 0;
            prevTime = currentTime;
        }

        static double lastFrameTime = glfwGetTime();
        double currentFrameTime = glfwGetTime();
        float deltaTime = static_cast<float>(currentFrameTime - lastFrameTime);
        deltaTime = glm::clamp(deltaTime, 0.0f, 0.1f);
        lastFrameTime = currentFrameTime;



        if (rainEnabled) { 
            if (!isFlashing) {
                nextFlashTime -= deltaTime;
                if (nextFlashTime <= 0.0f) {
                    isFlashing = true;
                    flashDuration = 0.3f + static_cast<float>(rand()) / RAND_MAX * 1.2f;
                    flashTimer = flashDuration;

                    myBasicShader.useShaderProgram();

                    activePulses.clear();
                    timeSinceLastPulse = 0.0f;

                    flashLight.enabled = true;
					glUniform1i(basicUniforms.flashLight.enabled, flashLight.enabled);

                    flashLight.direction = glm::normalize(glm::vec3(
                        static_cast<float>(rand()) / RAND_MAX * 2.0f - 1.0f,
                        static_cast<float>(rand()) / RAND_MAX * 0.5f + 0.5f,
                        static_cast<float>(rand()) / RAND_MAX * 2.0f - 1.0f
                    ));
					glUniform3fv(basicUniforms.flashLight.direction, 1, glm::value_ptr(flashLight.direction));

                    flashLight.color = glm::vec3(
                        10.0f + static_cast<float>(rand()) / RAND_MAX * 2.0f,
                        10.0f + static_cast<float>(rand()) / RAND_MAX * 2.0f,
                        12.0f + static_cast<float>(rand()) / RAND_MAX * 2.0f
                    );
					glUniform3fv(basicUniforms.flashLight.color, 1, glm::value_ptr(flashLight.color));

                    globalLightIntensity = 1.0f;
                    glUniform1f(basicUniforms.globalLightIntensity, globalLightIntensity);

                    isThunderScheduled = true;
                    thunderDelay = 0.8f + static_cast<float>(rand()) / RAND_MAX * 1.2f;
                    thunderTimer = thunderDelay;
                }
            }
            else { 
                flashTimer -= deltaTime;
                if (flashTimer <= 0.0f) {
                    isFlashing = false;

                    flashLight.enabled = false;
                    globalLightIntensity = 1.0f;

                    myBasicShader.useShaderProgram();
					glUniform1i(basicUniforms.flashLight.enabled, flashLight.enabled);

                    glUniform1f(basicUniforms.globalLightIntensity, globalLightIntensity);

                    activePulses.clear();
                    timeSinceLastPulse = 0.0f;

                    nextFlashTime = static_cast<float>(rand()) / RAND_MAX * (flashIntervalMax - flashIntervalMin) + flashIntervalMin;
                }

                timeSinceLastPulse += deltaTime;
                if (timeSinceLastPulse >= (static_cast<float>(rand()) / RAND_MAX * (pulseIntervalMax - pulseIntervalMin) + pulseIntervalMin)) {
                    float pulseIntensity = 2.0f + static_cast<float>(rand()) / RAND_MAX * 3.0f; 
                    float pulseDuration = 0.05f + static_cast<float>(rand()) / RAND_MAX * 0.15f; 
                    activePulses.push_back(IntensityPulse{ pulseDuration, pulseDuration, pulseIntensity });

                    timeSinceLastPulse = 0.0f;
                }
                updateGlobalLightIntensity(deltaTime);
            }
        }

        if (isThunderScheduled) {
            thunderTimer -= deltaTime;
            if (thunderTimer <= 0.0f) {
                audioManager.playThunder();
                isThunderScheduled = false;
            }
        }


        processMovement();
        updateRain(deltaTime);
        updateFire(deltaTime, static_cast<float>(currentTime));

        if (isTourActive) {
            updateTour();
        }

        renderScene();

        if (rainEnabled) {
            if (!rainPlaying) {
                audioManager.playRain(true);
                rainPlaying = true;
            }
        }
        else {
            if (rainPlaying) {
                audioManager.stopRain();
                rainPlaying = false;
            }
        }

        audioManager.updateListenerPosition(myCamera.getPosition(), myCamera.getFront(), myCamera.getUp());

        if (pointLight.enabled) {
            if (!firePlaying) {
                audioManager.playFire(true);
                firePlaying = true;
            }
        }
        else {
            if (firePlaying) {
                audioManager.stopFire();
                firePlaying = false;
            }
        }

        if (performHDR) {
            blurBrightTexture();

            glBindFramebuffer(GL_FRAMEBUFFER, 0);
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

            hdrShader.useShaderProgram();

            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, colorBuffers[0]);
			glUniform1i(hdrUniforms.hdrBuffer, 0);

            glActiveTexture(GL_TEXTURE1);

            bool finalTexIndex = (blurIterations % 2 == 0) ? 0 : 1;
            glBindTexture(GL_TEXTURE_2D, pingpongColorbuffers[finalTexIndex]);
			glUniform1i(hdrUniforms.bloomBlur, 1);

			glUniform1i(hdrUniforms.bloom, bloomEnabled);

			glUniform1f(hdrUniforms.exposure, exposure);

            renderQuad();
        }
        glfwPollEvents();
        glfwSwapBuffers(myWindow.getWindow());

        glCheckError();
    }
    cleanup();
    return EXIT_SUCCESS;
}
