#version 410 core

in vec2 TexCoord;
in vec4 ParticleColor;
flat in int texLayer;

out vec4 FragColor;

uniform sampler2DArray fireTextureArray;

void main()
{
    vec3 arrayCoord = vec3(TexCoord, float(texLayer));
    vec4 texSample = texture(fireTextureArray, arrayCoord);

    vec2 center = vec2(0.5, 0.5);
    float dist = length(vec2((TexCoord.x - 0.5) * 0.8, TexCoord.y - 0.5));

    float radialFade = 1.0 - smoothstep(0.30, 0.45, dist);

    float topFade = smoothstep(0.70, 1.00, TexCoord.y);

    float combinedFade = radialFade * (1.0 - topFade);

    vec4 outColor = ParticleColor * texSample * combinedFade;

    float glowAmount = outColor.a * 0.07;
    outColor.rgb += glowAmount;

    if (outColor.a < 0.02)
        discard;

    FragColor = outColor;
}
