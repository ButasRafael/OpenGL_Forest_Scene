#version 410 core

in vec2 position;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;
uniform vec3 cameraPos;
uniform vec3 lightPosition;

out vec4 clipSpace;
out vec2 texCoord;
out vec3 toCameraVector;
out vec3 fromLightVector;

const float tiling = 4.0;

void main()
{
    vec4 worldPos = model * vec4(position.x, 0.0, position.y, 1.0);

    clipSpace = projection * view * worldPos;
    gl_Position = clipSpace;

    texCoord = (vec2(position.x / 2.0 + 0.5, position.y / 2.0 + 0.5)) * tiling;

    toCameraVector = cameraPos - worldPos.xyz;
    fromLightVector = worldPos.xyz - lightPosition;
}
