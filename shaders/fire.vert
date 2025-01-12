#version 410 core

layout (location = 0) in vec2 inQuadPos;
layout (location = 1) in vec2 inTexCoord;
layout (location = 2) in vec3 inPosition;
layout (location = 3) in vec4 inColor;
layout (location = 4) in float inSize;
layout (location = 5) in float inRotation;
layout (location = 6) in int   inTexIndex;

out vec2 TexCoord;
out vec4 ParticleColor;
flat out int texLayer;

uniform mat4 view;
uniform mat4 projection;
uniform vec3 cameraRight;
uniform vec3 cameraUp;
uniform float flameAspectX;
uniform float flameAspectY;
uniform float time;

void main()
{
    float rad  = radians(inRotation);
    float cosT = cos(rad);
    float sinT = sin(rad);
    vec2 rotatedPos = vec2(
        inQuadPos.x * cosT - inQuadPos.y * sinT,
        inQuadPos.x * sinT + inQuadPos.y * cosT
    );

    if (inQuadPos.y > 0.3) {
        float swirl = sin((inQuadPos.x + time * 3.0) * 10.0) * 0.03;
        rotatedPos.x += swirl * (inQuadPos.y - 0.3) * 1.5;
    }

    float heightFactor = clamp((inQuadPos.y + 0.5) / 1.0, 0.0, 1.0);
    float taper = mix(1.0, 0.75, heightFactor);
    vec2 taperedPos = rotatedPos * vec2(taper, 1.0);

    float distortion = sin(taperedPos.x * 12.0 + time * 6.0) * 0.02;
    taperedPos.y += distortion;

    vec3 worldPos = inPosition
        + cameraRight * (taperedPos.x * inSize * flameAspectX)
        + cameraUp    * (taperedPos.y * inSize * flameAspectY);

    gl_Position   = projection * view * vec4(worldPos, 1.0);
    TexCoord      = inTexCoord;
    ParticleColor = inColor;
    texLayer      = inTexIndex;
}
