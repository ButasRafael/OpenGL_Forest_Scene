#version 410 core

in vec3 fPosition;
in vec3 fNormal;
in vec2 fTexCoords;
in mat3 TBN;
in vec4 FragPosLightSpace;
in vec4 FragPosLightSpaceLeftHeadlight;
in vec4 FragPosLightSpaceRightHeadlight;

layout(location = 0) out vec4 gFragColor;
layout(location = 1) out vec4 gBrightColor;


// Lighting structures
struct DirLight {
    vec3 direction;
    vec3 ambient;
    vec3 diffuse;
    vec3 specular;
    vec3 color;
    int enabled;
};

struct PointLight {
    vec3 position;
    vec3 ambient;
    vec3 diffuse;
    vec3 specular;
    vec3 color;
    float constant;
    float linear;
    float quadratic;
    int enabled;
};

struct SpotLight {
    vec3 position;
    vec3 direction;
    float cutOff;
    float outerCutOff;
    vec3 ambient;
    vec3 diffuse;
    vec3 specular;
    vec3 color;
    int enabled;
};

uniform DirLight dirLight;
uniform DirLight flashLight;
uniform PointLight pointLight;
uniform SpotLight spotLight;
uniform SpotLight leftHeadlight;
uniform SpotLight rightHeadlight;
uniform vec3 viewPos;
uniform int useBlinnPhong;
uniform int useNormalMapping;
uniform int rainEnabled;

uniform sampler2D diffuseTexture;
uniform sampler2D normalTexture;
uniform sampler2D specularTexture;
uniform sampler2D dissolveTexture;
uniform sampler2D shadowMap;
uniform samplerCube pointLightShadowMap;
uniform sampler2D leftHeadlightShadowMap;
uniform sampler2D rightHeadlightShadowMap;

uniform float farPlane;

uniform float globalLightIntensity;

uniform int fogEnabled;
uniform vec3 fogColor;
uniform float gFogEnd;
uniform float gLayeredFogTop;
uniform float gFogTime;
uniform float gExpFogDensity;

uniform int windEnabled;

vec3 sampleOffsetDirections[20] = vec3[](
    vec3( 1,  1,  1), vec3( 1, -1,  1), vec3(-1, -1,  1), vec3(-1,  1,  1),
    vec3( 1,  1, -1), vec3( 1, -1, -1), vec3(-1, -1, -1), vec3(-1,  1, -1),
    vec3( 1,  1,  0), vec3( 1, -1,  0), vec3(-1, -1,  0), vec3(-1,  1,  0),
    vec3( 1,  0,  1), vec3(-1,  0,  1), vec3( 1,  0, -1), vec3(-1,  0, -1),
    vec3( 0,  1,  1), vec3( 0, -1,  1), vec3( 0, -1, -1), vec3( 0,  1, -1)
);

float CalcLayeredFogFactor()
{
    vec3 CameraProj = viewPos;
    CameraProj.y = 0.0;

    vec3 PixelProj = fPosition;
    PixelProj.y = 0.0;

    float DeltaD = length(CameraProj - PixelProj) / gFogEnd;

    float DeltaY = 0.0;
    float DensityIntegral = 0.0;

    if (viewPos.y > gLayeredFogTop) {
        if (fPosition.y < gLayeredFogTop) {
            DeltaY = (gLayeredFogTop - fPosition.y) / gLayeredFogTop;
            DensityIntegral = DeltaY * DeltaY * 0.5;
        }
    } else {  
        if (fPosition.y < gLayeredFogTop) { 
            DeltaY = abs(viewPos.y - fPosition.y) / gLayeredFogTop;
            float DeltaCamera = (gLayeredFogTop - viewPos.y) / gLayeredFogTop;
            float DensityIntegralCamera = DeltaCamera * DeltaCamera * 0.5;
            float DeltaPixel = (gLayeredFogTop - fPosition.y) / gLayeredFogTop;
            float DensityIntegralPixel = DeltaPixel * DeltaPixel * 0.5;
            DensityIntegral = abs(DensityIntegralCamera - DensityIntegralPixel);
        } else {
            DeltaY = (gLayeredFogTop - viewPos.y) / gLayeredFogTop;
            DensityIntegral = DeltaY * DeltaY * 0.5;
        }
    }

    float FogDensity = 0.0;

    if (DeltaY != 0) {
        FogDensity = (sqrt(1.0 + ((DeltaD / DeltaY) * (DeltaD / DeltaY)))) * DensityIntegral;
    }

    float FogFactor = exp(-FogDensity);

    return FogFactor;
}

#define PI 3.1415926535897932384626433832795

float CalcAnimatedFogFactor()
{
    float CameraToPixelDist = length(fPosition - viewPos);

    float DistRatio = CameraToPixelDist / gFogEnd;

    float ExpFogFactor = exp(-DistRatio * gExpFogDensity);

    float x = fPosition.x / 20.0;
    float y = fPosition.y / 20.0;
    float z = fPosition.z / 20.0;

    float AnimFactor = -(1.0 +
                         0.5 * cos(5.0 * PI * z + gFogTime) +
                         0.2 * cos(7.0 * PI * (z + 0.1 * x)) +
                         0.2 * cos(5.0 * PI * (z - 0.05 * x)) +
                         0.1 * cos(PI * x) * cos(PI * z / 2.0));

    float FogFactor = ExpFogFactor + (CameraToPixelDist / gFogEnd) * AnimFactor;

    return FogFactor;
}



float PointShadowCalculation(vec3 normal, vec3 fragPos, vec3 lightDir)
{
    vec3 fragToLight = fragPos - pointLight.position;
    
    float currentDepth = length(fragToLight);
    
    float bias = max(0.05 * (1.0f - dot(normal,lightDir)), 0.0005);
    
    float viewDistance = length(viewPos - fragPos);
    
    float diskRadius = (1.0 + (viewDistance / farPlane)) / 25.0;
    
    float shadow = 0.0;
    
    int samples = 20;
    for (int i = 0; i < samples; ++i)
    {
        float closestDepth = texture(pointLightShadowMap, fragToLight + sampleOffsetDirections[i] * diskRadius).r;
        closestDepth *= farPlane;
        
        if (currentDepth - bias > closestDepth)
            shadow += 1.0;
    }
    
    shadow /= float(samples);

    if (spotLight.enabled == 1) {
        vec3 spotDir = normalize(spotLight.position - fragPos);
        float theta = dot(spotDir, normalize(-spotLight.direction));
        float epsilon = spotLight.cutOff - spotLight.outerCutOff;
        float intensity = clamp((theta - spotLight.outerCutOff) / epsilon, 0.0, 1.0);

        shadow *= 1.0 - intensity;
    }
    
    return shadow;
}



float ShadowCalculation(vec3 normal, vec4 fragPosLightSpace, vec3 fragPos, vec3 lightDir) {
    vec3 projCoords = fragPosLightSpace.xyz / fragPosLightSpace.w;
    projCoords = projCoords * 0.5 + 0.5;

    if (projCoords.z > 1.0) return 0.0;

    float bias = max(0.025 * (1.0f - dot(normal,lightDir)), 0.0005);
    float shadow = 0.0;
    int sampleRadius = 3;
    vec2 texelSize = 1.0 / textureSize(shadowMap, 0);

    for (int y = -sampleRadius; y <= sampleRadius; y++) {
        for (int x = -sampleRadius; x <= sampleRadius; x++) {
            float closestDepth = texture(shadowMap, projCoords.xy + vec2(x, y) * texelSize).r;
            shadow += projCoords.z - bias > closestDepth ? 1.0 : 0.0;
        }
    }
    shadow /= pow((sampleRadius * 2 + 1), 2);

    if (spotLight.enabled == 1) {
        vec3 spotDir = normalize(spotLight.position - fragPos);
        float theta = dot(spotDir, normalize(-spotLight.direction));
        float epsilon = spotLight.cutOff - spotLight.outerCutOff;
        float intensity = clamp((theta - spotLight.outerCutOff) / epsilon, 0.0, 1.0);

        shadow *= 1.0 - intensity;
    }

    return shadow;
}


float HeadlightShadowCalculation(vec3 normal, vec4 fragPosLightSpace, vec3 fragPos, vec3 lightDir, sampler2D headlightShadowMap) {
    vec3 projCoords = fragPosLightSpace.xyz / fragPosLightSpace.w;
    projCoords = projCoords * 0.5 + 0.5;

    if (projCoords.z > 1.0) return 0.0;
    float bias = max(0.00025 * (1.0f - dot(normal, lightDir)), 0.000005); 

    float shadow = 0.0;
    int sampleRadius = 3;
    vec2 texelSize = 1.0 / textureSize(headlightShadowMap, 0);

    for (int y = -sampleRadius; y <= sampleRadius; y++) {
        for (int x = -sampleRadius; x <= sampleRadius; x++) {
            float closestDepth = texture(headlightShadowMap, projCoords.xy + vec2(x, y) * texelSize).r;
            shadow += projCoords.z - bias > closestDepth ? 1.0 : 0.0;
        }
    }
    shadow /= pow((sampleRadius * 2 + 1), 2);

    if (spotLight.enabled == 1) {
        vec3 spotDir = normalize(spotLight.position - fragPos);
        float theta = dot(spotDir, normalize(-spotLight.direction));
        float epsilon = spotLight.cutOff - spotLight.outerCutOff;
        float intensity = clamp((theta - spotLight.outerCutOff) / epsilon, 0.0, 1.0);

        shadow *= 1.0 - intensity;
    }

    return shadow;
}





vec3 computeAmbientDirLight(vec3 normal) {
    if (dirLight.enabled == 0) return vec3(0.0);
    vec3 ambient = dirLight.ambient * dirLight.color;
    return ambient;
}

vec3 computeDiffuseDirLight(vec3 normal, vec3 lightDir) {
    if (dirLight.enabled == 0) return vec3(0.0);
    float diff = max(dot(normal, lightDir), 0.0);
    vec3 diffuse = diff * dirLight.diffuse * dirLight.color;
    return diffuse;
}

vec3 computeSpecularDirLight(vec3 normal, vec3 viewDir, vec3 lightDir) {
    vec3 halfDir = normalize(lightDir + viewDir);
    float spec = pow(max(dot(normal, halfDir), 0.0), 32.0);
    return dirLight.specular * spec;
}

vec3 computeAmbientPointLight(vec3 normal, vec3 fragPos) {
    if (pointLight.enabled == 0) return vec3(0.0);
    float distance = length(pointLight.position - fragPos);
    float attenuation = 1.0 / (pointLight.constant + pointLight.linear * distance + pointLight.quadratic * (distance * distance));
    vec3 ambient = pointLight.ambient * pointLight.color * attenuation;
    return ambient;
}

vec3 computeDiffusePointLight(vec3 normal, vec3 fragPos, vec3 lightDir) {
    if (pointLight.enabled == 0) return vec3(0.0);
    float distance = length(pointLight.position - fragPos);
    float attenuation = 1.0 / (pointLight.constant + pointLight.linear * distance + pointLight.quadratic * (distance * distance));
    float diff = max(dot(normal, lightDir), 0.0);
    vec3 diffuse = pointLight.diffuse * pointLight.color * diff * attenuation;
    return diffuse;
}

vec3 computeSpecularPointLight(vec3 normal, vec3 fragPos, vec3 viewDir, vec3 lightDir) {
    vec3 halfDir = normalize(lightDir + viewDir);
    float distance = length(pointLight.position - fragPos);
    float attenuation = 1.0 / (pointLight.constant + pointLight.linear * distance + pointLight.quadratic * (distance * distance));
    float spec = pow(max(dot(normal, halfDir), 0.0), 32.0);
    return pointLight.specular * spec * attenuation;
}

vec3 computeAmbientSpotLight(vec3 normal, vec3 fragPos) {
    if (spotLight.enabled == 0) return vec3(0.0);
    vec3 ambient = spotLight.ambient * spotLight.color;
    return ambient;
}

vec3 computeDiffuseSpotLight(vec3 normal, vec3 fragPos, vec3 lightDir) {
    if (spotLight.enabled == 0) return vec3(0.0);
    float theta = dot(lightDir, normalize(-spotLight.direction));
    float epsilon = spotLight.cutOff - spotLight.outerCutOff;
    float intensity = clamp((theta - spotLight.outerCutOff) / epsilon, 0.0, 1.0);

    float diff = max(dot(normal, lightDir), 0.0);
    vec3 diffuse = spotLight.diffuse * spotLight.color * diff * intensity;
    return diffuse;
}

vec3 computeSpecularSpotLight(vec3 normal, vec3 fragPos, vec3 viewDir, vec3 lightDir) {
    vec3 halfDir = normalize(lightDir + viewDir);
    float theta = dot(lightDir, normalize(-spotLight.direction));
    float epsilon = spotLight.cutOff - spotLight.outerCutOff;
    float intensity = clamp((theta - spotLight.outerCutOff) / epsilon, 0.0, 1.0);
    float spec = pow(max(dot(normal, halfDir), 0.0), 32.0);
    return spotLight.specular * spec * intensity;
}

vec3 computeAmbientLeftHeadLight(vec3 normal, vec3 fragPos) {
    if (leftHeadlight.enabled == 0) return vec3(0.0);
    vec3 ambient = leftHeadlight.ambient * leftHeadlight.color;
    return ambient;
}

vec3 computeDiffuseLeftLight(vec3 normal, vec3 fragPos, vec3 lightDir) {
    if (leftHeadlight.enabled == 0) return vec3(0.0);
    float theta = dot(lightDir, normalize(-leftHeadlight.direction));
    float epsilon = leftHeadlight.cutOff - leftHeadlight.outerCutOff;
    float intensity = clamp((theta - leftHeadlight.outerCutOff) / epsilon, 0.0, 1.0);

    float diff = max(dot(normal, lightDir), 0.0);
    vec3 diffuse = leftHeadlight.diffuse * leftHeadlight.color * diff * intensity;
    return diffuse;
}

vec3 computeSpecularLeftHeadLight(vec3 normal, vec3 fragPos, vec3 viewDir, vec3 lightDir) {
    vec3 halfDir = normalize(lightDir + viewDir);
    float theta = dot(lightDir, normalize(-leftHeadlight.direction));
    float epsilon = leftHeadlight.cutOff - leftHeadlight.outerCutOff;
    float intensity = clamp((theta - leftHeadlight.outerCutOff) / epsilon, 0.0, 1.0);
    float spec = pow(max(dot(normal, halfDir), 0.0), 32.0);
    return leftHeadlight.specular * spec * intensity;
}

vec3 computeAmbientRightHeadLight(vec3 normal, vec3 fragPos) {
    if (rightHeadlight.enabled == 0) return vec3(0.0);
    vec3 ambient = rightHeadlight.ambient * rightHeadlight.color;
    return ambient;
}

vec3 computeDiffuseRightLight(vec3 normal, vec3 fragPos, vec3 lightDir) {
    if (rightHeadlight.enabled == 0) return vec3(0.0);
    float theta = dot(lightDir, normalize(-rightHeadlight.direction));
    float epsilon = rightHeadlight.cutOff - rightHeadlight.outerCutOff;
    float intensity = clamp((theta - rightHeadlight.outerCutOff) / epsilon, 0.0, 1.0);

    float diff = max(dot(normal, lightDir), 0.0);
    vec3 diffuse = rightHeadlight.diffuse * rightHeadlight.color * diff * intensity;
    return diffuse;
}

vec3 computeSpecularRightHeadLight(vec3 normal, vec3 fragPos, vec3 viewDir, vec3 lightDir) {
    vec3 halfDir = normalize(lightDir + viewDir);
    float theta = dot(lightDir, normalize(-rightHeadlight.direction));
    float epsilon = rightHeadlight.cutOff - rightHeadlight.outerCutOff;
    float intensity = clamp((theta - rightHeadlight.outerCutOff) / epsilon, 0.0, 1.0);
    float spec = pow(max(dot(normal, halfDir), 0.0), 32.0);
    return rightHeadlight.specular * spec * intensity;
}

void main() {
    vec3 normal = fNormal;

    if (useNormalMapping == 1) {
        normal = texture(normalTexture, fTexCoords).rgb * 2.0 - 1.0;
        normal = normalize(TBN * normal);
    }

    vec3 viewDir = normalize(viewPos - fPosition);

    vec3 DirectionalLightDir = normalize(-dirLight.direction);
    vec3 PointLightDir = normalize(pointLight.position - fPosition);
    vec3 SpotLightDir = normalize(spotLight.position - fPosition);
    vec3 LeftHeadlightDir = normalize(leftHeadlight.position - fPosition);
    vec3 RightHeadlightDir = normalize(rightHeadlight.position - fPosition);

    vec3 textureColor = texture(diffuseTexture, fTexCoords).rgb;
    vec3 specularColor = texture(specularTexture, fTexCoords).rgb;
    vec4 dissolveColor = texture(dissolveTexture, fTexCoords);
    if (dissolveColor.a < 0.1) discard;

    float dirShadow = ShadowCalculation(normal, FragPosLightSpace, fPosition, DirectionalLightDir);
    float pointShadow = PointShadowCalculation(normal, fPosition, PointLightDir);  // Added Point Light Shadow Calculation
    float leftHeadlightShadow = HeadlightShadowCalculation(normal, FragPosLightSpaceLeftHeadlight, fPosition, LeftHeadlightDir, leftHeadlightShadowMap);
    float rightHeadlightShadow = HeadlightShadowCalculation(normal, FragPosLightSpaceRightHeadlight, fPosition, RightHeadlightDir, rightHeadlightShadowMap);

    vec3 finalResult = vec3(0.0);

    if (dirLight.enabled == 1) {
        vec3 ambient = computeAmbientDirLight(normal);
        vec3 diffuse = computeDiffuseDirLight(normal, DirectionalLightDir);
        vec3 specular = vec3(0.0);
        if(useBlinnPhong == 1) {
            specular = computeSpecularDirLight(normal, viewDir, DirectionalLightDir);
        }
        else if (rainEnabled == 1) {
            specular = computeSpecularDirLight(normal, viewDir, DirectionalLightDir);
        }
        
        vec3 dirLightResult = ambient * textureColor;
        dirLightResult += (1.0 - dirShadow) * (diffuse * textureColor + specular * specularColor);
        finalResult += dirLightResult;
    }

    if (pointLight.enabled == 1) {
        vec3 ambient = computeAmbientPointLight(normal, fPosition);
        vec3 diffuse = computeDiffusePointLight(normal, fPosition, PointLightDir);
        vec3 specular = vec3(0.0);
        if (useBlinnPhong == 1) {
            specular = computeSpecularPointLight(normal, fPosition, viewDir, PointLightDir);
        }
        else if (rainEnabled == 1) {
            specular = computeSpecularPointLight(normal, fPosition, viewDir, PointLightDir);
		}

        vec3 pointLightResult = ambient * textureColor;
        pointLightResult += (1.0 - pointShadow) * (diffuse * textureColor + specular * specularColor);
        finalResult += pointLightResult;
    }

    if (spotLight.enabled == 1) {
        vec3 ambient = computeAmbientSpotLight(normal, fPosition);
        vec3 diffuse = computeDiffuseSpotLight(normal, fPosition, SpotLightDir);
        vec3 specular = vec3(0.0);
        if(useBlinnPhong == 1) {
            specular = computeSpecularSpotLight(normal, fPosition, viewDir, SpotLightDir);
        }
        else if (rainEnabled == 1) {
            specular = computeSpecularSpotLight(normal, fPosition, viewDir, SpotLightDir);
        }

        vec3 spotLightResult = ambient * textureColor;
        spotLightResult += (diffuse * textureColor + specular * specularColor);
        finalResult += spotLightResult;
    }

    if (leftHeadlight.enabled == 1) {
        vec3 ambient = computeAmbientLeftHeadLight(normal, fPosition);
        vec3 diffuse = computeDiffuseLeftLight(normal, fPosition, LeftHeadlightDir);
        vec3 specular = vec3(0.0);
        if(useBlinnPhong == 1) {
            specular = computeSpecularLeftHeadLight(normal, fPosition, viewDir, LeftHeadlightDir);
        }
        else if (rainEnabled == 1) {
			specular = computeSpecularLeftHeadLight(normal, fPosition, viewDir, LeftHeadlightDir);
		}

        vec3 leftHeadlightResult = ambient * textureColor;
        leftHeadlightResult += (1.0 - leftHeadlightShadow) * (diffuse * textureColor + specular * specularColor);
        finalResult += leftHeadlightResult;
    }

    if (rightHeadlight.enabled == 1) {
        vec3 ambient = computeAmbientRightHeadLight(normal, fPosition);
        vec3 diffuse = computeDiffuseRightLight(normal, fPosition, RightHeadlightDir);
        vec3 specular = vec3(0.0);
        if(useBlinnPhong == 1){
            specular = computeSpecularRightHeadLight(normal, fPosition, viewDir, RightHeadlightDir);
        }
        else if (rainEnabled == 1) {
			specular = computeSpecularRightHeadLight(normal, fPosition, viewDir, RightHeadlightDir);
            }

        vec3 rightHeadlightResult = ambient * textureColor;
        rightHeadlightResult += (1.0 - rightHeadlightShadow) * (diffuse * textureColor + specular * specularColor);
        finalResult += rightHeadlightResult;
    }

     if (flashLight.enabled == 1) {
        vec3 ambient = flashLight.ambient * flashLight.color;
        vec3 diffuse = computeDiffuseDirLight(normal, normalize(-flashLight.direction));
        vec3 specular = vec3(0.0);
        if(useBlinnPhong == 1) {
            specular = computeSpecularDirLight(normal, viewDir, normalize(-flashLight.direction));
        }
        else if (rainEnabled == 1) {
            specular = computeSpecularDirLight(normal, viewDir, normalize(-flashLight.direction));
        }

        vec3 flashLightResult = ambient * textureColor;
        flashLightResult += diffuse * textureColor + specular * specularColor;
        finalResult += flashLightResult;
    }

    if (fogEnabled == 1) {
        float fogFactor = 0.0;

        if (windEnabled == 1) {
            fogFactor = CalcAnimatedFogFactor(); // Use animated fog when wind is enabled
        } else {
            fogFactor = CalcLayeredFogFactor(); // Use layered fog otherwise
        }

        // Clamp the fog factor to ensure it's within valid range
        fogFactor = clamp(fogFactor, 0.0, 1.0);

        // Apply the fog effect to the final result
        finalResult = mix(fogColor, finalResult, fogFactor);
}

     finalResult *= globalLightIntensity;

     if (flashLight.enabled == 1) {
        vec3 flashTint = vec3(0.5, 0.5, 1.0);
        float tintStrength = 0.3;
        finalResult = mix(finalResult, flashTint * finalResult, tintStrength);
    }

    gFragColor = vec4(finalResult, 1.0);

    float brightness = dot(finalResult, vec3(0.2126, 0.7152, 0.0722));

    float threshold = 1.0;

    if (brightness > threshold) {
        gBrightColor = vec4(finalResult, 1.0);
    } else {
        gBrightColor = vec4(0.0);
    }

}
