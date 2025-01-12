#version 410 core

layout (location = 0) in vec3 inPosition;
layout (location = 1) in vec2 inParams;

uniform mat4 view;
uniform mat4 projection;

out vec3 vPosition;
out float vLengthFactor;
out float vSpeedFactor;

void main() {
    vPosition = inPosition;
    vLengthFactor = inParams.x;
    vSpeedFactor = inParams.y;
    gl_Position = projection * view * vec4(inPosition, 1.0);
}
