#version 410 core
in vec3 TexCoords;
out vec4 FragColor;

uniform samplerCube daySkybox;
uniform samplerCube nightSkybox;
uniform float blendFactor;

void main() {
    vec4 dayColor = texture(daySkybox, TexCoords);
    vec4 nightColor = texture(nightSkybox, TexCoords);
    FragColor = mix(nightColor, dayColor, blendFactor);
}
