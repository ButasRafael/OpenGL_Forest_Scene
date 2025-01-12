#version 410 core
out vec4 FragColor;
in vec2 TexCoords;

uniform sampler2D hdrBuffer;
uniform sampler2D bloomBlur;
uniform int bloom;
uniform float exposure;

void main() 
{
    vec3 hdrColor = texture(hdrBuffer, TexCoords).rgb;

    vec3 bloomColor = texture(bloomBlur, TexCoords).rgb;

    vec3 color = hdrColor;
    if(bloom == 1) {
        color += bloomColor;
    }

    vec3 mapped = vec3(1.0) - exp(-color * exposure);

    float gamma = 2.2;
    vec3 gammaCorrected = pow(mapped, vec3(1.0 / gamma));

    FragColor = vec4(gammaCorrected, 1.0);
}
