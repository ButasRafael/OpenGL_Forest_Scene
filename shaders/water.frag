#version 410 core

in vec4 clipSpace;
in vec2 texCoord;
in vec3 toCameraVector;
in vec3 fromLightVector;

out vec4 FragColor;
uniform sampler2D reflectionTexture;
uniform sampler2D refractionTexture;
uniform sampler2D dudvMap;
uniform sampler2D normalMap;
uniform sampler2D depthMap;
uniform vec3 lightColor;
uniform float moveFactor;

const float waveStrength = 0.04;
const float shineDamper = 20.0;
const float reflectivity = 0.5;
const vec4 mudColor = vec4(0.22, 0.35, 0.25, 0.6);


void main()
{
	vec2 ndc = (clipSpace.xy / clipSpace.w) / 2.0 + 0.5;
    vec2 reflectionTexCoord = vec2(ndc.x, 1.0 - ndc.y);
    vec2 refractionTexCoord = vec2(ndc.x, ndc.y);

    float near = 0.1;
    float far = 300.0;
    float floorDepth = texture(depthMap, refractionTexCoord).r;
    float floorDistance = 2.0 * near * far / (far + near - (2.0 * floorDepth - 1.0) * (far - near));
    float fragmentDepth = gl_FragCoord.z;
    float fragmentDistance = 2.0 * near * far / (far + near - (2.0 * fragmentDepth - 1.0) * (far - near));
    float waterDepth = floorDistance - fragmentDistance;


    vec2 distortedTexCoords = texture(dudvMap, vec2(texCoord.x + moveFactor, texCoord.y)).rg * 0.1;
	distortedTexCoords = texCoord + vec2(distortedTexCoords.x, distortedTexCoords.y + moveFactor);
	vec2 totalDistortion = (texture(dudvMap, distortedTexCoords).rg * 2.0 - 1.0) * waveStrength * clamp(waterDepth / 20.0, 0.0, 1.0);


    reflectionTexCoord += totalDistortion;
    reflectionTexCoord = clamp(reflectionTexCoord, 0.001, 0.999);
    refractionTexCoord += totalDistortion;
    refractionTexCoord = clamp(refractionTexCoord, 0.001, 0.999);

    vec4 reflectionColor = texture(reflectionTexture, reflectionTexCoord);
    vec4 refractionColor = texture(refractionTexture, refractionTexCoord);
    refractionColor = mix(refractionColor, mudColor, clamp(waterDepth / 60.0, 0.0, 1.0));

    vec4 normalColor = texture(normalMap, distortedTexCoords);
    vec3 normal = vec3(normalColor.r * 2.0 - 1.0, normalColor.b * 3.0, normalColor.g * 2.0 - 1.0);
    normal = normalize(normal);

    
    vec3 viewVector = normalize(toCameraVector);
    float refractiveFactor = dot(viewVector, normal);
    refractiveFactor = pow(refractiveFactor, 0.5);
    refractiveFactor = clamp(refractiveFactor, 0.0, 1.0);


    vec3 reflectedLight = reflect(normalize(fromLightVector), normal);
    float specular = max(dot(reflectedLight, viewVector), 0.0);
    specular = pow(specular, shineDamper);
    vec3 specularHighlights = lightColor * specular * reflectivity * clamp(waterDepth / 5.0, 0.0, 1.0);

    FragColor = mix(reflectionColor, refractionColor, refractiveFactor);
    FragColor = mix(FragColor, vec4(0.0, 0.3, 0.5, 1.0), 0.2) + vec4(specularHighlights, 0.0);

    FragColor.a = clamp(waterDepth / 5.0, 0.0, 1.0);


}