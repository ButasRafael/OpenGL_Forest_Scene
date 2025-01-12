#version 410 core
layout(points) in;
layout(triangle_strip, max_vertices = 4) out;

in vec4 ParticleColor[];
out vec4 FragColor;

uniform sampler2D splashTexture;

void main()
{
    FragColor = ParticleColor[0];

    // Define a quad around the point
    float size = gl_PointSize / 100.0; // Adjust size scaling as needed

    // Corner offsets
    vec2 offset[4] = vec2[](
        vec2(-size, size),
        vec2(-size, -size),
        vec2(size, size),
        vec2(size, -size)
    );

    for(int i = 0; i < 4; ++i)
    {
        vec4 pos = gl_in[0].gl_Position;
        pos.xy += offset[i];
        gl_Position = pos;
        FragColor = ParticleColor[0];
        EmitVertex();
    }
    EndPrimitive();
}
