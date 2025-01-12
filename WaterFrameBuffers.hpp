
#ifndef WATER_FRAME_BUFFERS_HPP
#define WATER_FRAME_BUFFERS_HPP

#include <GL/glew.h>
#include <iostream>

class WaterFrameBuffers {
public:
    WaterFrameBuffers(int reflectionWidth, int reflectionHeight, int refractionWidth, int refractionHeight);

    ~WaterFrameBuffers();

    void bindReflectionFrameBuffer();

    void bindRefractionFrameBuffer();

    void unbindCurrentFrameBuffer(int windowWidth, int windowHeight);

    GLuint getReflectionTexture() const { return reflectionTexture; }
    GLuint getRefractionTexture() const { return refractionTexture; }
    GLuint getRefractionDepthTexture() const { return refractionDepthTexture; }
    void resize(int newWidth, int newHeight);

private:
    GLuint reflectionFrameBuffer;
    GLuint refractionFrameBuffer;

    GLuint reflectionTexture;
    GLuint reflectionDepthBuffer;

    GLuint refractionTexture;
    GLuint refractionDepthTexture;

    int REFLECTION_WIDTH;
    int REFLECTION_HEIGHT;
    int REFRACTION_WIDTH;
    int REFRACTION_HEIGHT;

    void initialiseReflectionFrameBuffer();
    void initialiseRefractionFrameBuffer();

    void bindFrameBuffer(GLuint frameBuffer, int width, int height);
    GLuint createFrameBuffer();
    GLuint createTextureAttachment(int width, int height);
    GLuint createDepthTextureAttachment(int width, int height);
    GLuint createDepthBufferAttachment(int width, int height);
};

#endif
