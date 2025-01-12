#version 410 core

layout(points) in;
layout(triangle_strip, max_vertices = 4) out;

in vec3 vPosition[];
in float vLengthFactor[];
in float vSpeedFactor[];

uniform float baseDropLength = 0.2;
uniform float curvature = 0.1;
uniform float maxThickness = 0.02;

uniform vec3 viewPosition;
uniform float maxDistance = 50.0;

out float vDropPos;
out float vSpeedFactorOut;
out float vDistance;
out vec3 vNormal;
out vec3 fragWorldPos;

void main() {
    float dropLength = baseDropLength * vLengthFactor[0] * (1.0 + vSpeedFactor[0] / 2.0);

    vec4 P = gl_in[0].gl_Position;

    vec3 worldPosition = vPosition[0];

    float distance = length(worldPosition - viewPosition);
    vDistance = distance;

    vec3 direction = vec3(0.0, -1.0, 0.0);
    vec3 perpendicular = vec3(1.0, 0.0, 0.0);

    float thickness = maxThickness * clamp(vSpeedFactor[0] / 2.0, 0.5, 1.0);

    vec3 offset = perpendicular * thickness;

    vec3 normal = normalize(viewPosition - worldPosition);
    vNormal = normal;

    vDropPos = 0.0;
    vSpeedFactorOut = vSpeedFactor[0];
    fragWorldPos = worldPosition;
    gl_Position = P;
    EmitVertex();

    vec4 P_mid_left = P + vec4(-offset.x, -dropLength / 2.0 + curvature, 0.0, 0.0);
    vDropPos = 0.5;
    vSpeedFactorOut = vSpeedFactor[0];
    fragWorldPos = worldPosition + vec3(-offset.x, -dropLength / 2.0 + curvature, 0.0);
    gl_Position = P_mid_left;
    EmitVertex();

    vec4 P_mid_right = P + vec4(offset.x, -dropLength / 2.0 + curvature, 0.0, 0.0);
    vDropPos = 0.5;
    vSpeedFactorOut = vSpeedFactor[0];
    fragWorldPos = worldPosition + vec3(offset.x, -dropLength / 2.0 + curvature, 0.0);
    gl_Position = P_mid_right;
    EmitVertex();

    vec4 P_bottom = P + vec4(0.0, -dropLength, 0.0, 0.0);
    vDropPos = 1.0;
    vSpeedFactorOut = vSpeedFactor[0];
    fragWorldPos = worldPosition + vec3(0.0, -dropLength, 0.0);
    gl_Position = P_bottom;
    EmitVertex();

    EndPrimitive();
}
