#version 410 core
layout (location = 0) in vec3 vPos;

uniform mat4 lightSpaceMatrixSpot;
uniform mat4 model;

void main()
{
    gl_Position = lightSpaceMatrixSpot * model * vec4(vPos, 1.0);
}  