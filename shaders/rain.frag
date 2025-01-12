#version 410 core

in float vDropPos;
in float vSpeedFactorOut;
in float vDistance;
in vec3 vNormal;
in vec3 fragWorldPos;

out vec4 FragColor;

uniform vec4 rainColor = vec4(0.7, 0.7, 1.0, 0.5);
uniform float maxDistance = 50.0;
uniform vec3 viewPosition;

struct DirLight {
    vec3 direction;
    vec3 ambient;
    vec3 diffuse;
    vec3 specular;
    vec3 color;
    bool enabled;
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
    bool enabled;
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
    bool enabled;
};

struct Headlight {
    vec3 position;
    vec3 direction;
    float cutOff;
    float outerCutOff;
    vec3 ambient;
    vec3 diffuse;
    vec3 specular;
    vec3 color;
    bool enabled;
};


uniform DirLight dirLight;
uniform DirLight flashLight;
uniform PointLight pointLight;
uniform SpotLight spotLight;
uniform Headlight leftHeadlight;
uniform Headlight rightHeadlight;

uniform float shininess = 32.0;
uniform float motionBlurIntensity = 0.7;


uniform samplerCube environmentMap;

vec3 CalcDirLight(DirLight light, vec3 norm, vec3 viewDir);
vec3 CalcPointLight(PointLight light, vec3 norm, vec3 fragPos, vec3 viewDir);
vec3 CalcSpotLight(SpotLight light, vec3 norm, vec3 fragPos, vec3 viewDir);
vec3 CalcHeadlight(Headlight light, vec3 norm, vec3 fragPos, vec3 viewDir);

void main() {
    float alpha = 1.0;
    if (vDropPos < 0.25) {
        alpha = mix(0.0, 1.0, vDropPos / 0.25);
    } else if (vDropPos > 0.75) {
        alpha = mix(1.0, 0.0, (vDropPos - 0.75) / 0.25);
    }

    alpha *= motionBlurIntensity * (1.0 - vDropPos);
    float alphaSpeed = clamp(vSpeedFactorOut / 2.0, 0.5, 1.0);
    float alphaDepth = clamp(1.0 - (vDistance / maxDistance), 0.0, 1.0);
    alpha *= alphaSpeed * alphaDepth;

    vec3 norm = normalize(vNormal);
    vec3 viewDir = normalize(viewPosition - fragWorldPos);

    vec3 lighting = vec3(0.0);

    if (dirLight.enabled) {
        lighting += CalcDirLight(dirLight, norm, viewDir);
    }

    if (pointLight.enabled) {
        lighting += CalcPointLight(pointLight, norm, fragWorldPos, viewDir);
    }

    if (spotLight.enabled) {
        lighting += CalcSpotLight(spotLight, norm, fragWorldPos, viewDir);
    }

    if (leftHeadlight.enabled) {
        lighting += CalcHeadlight(leftHeadlight, norm, fragWorldPos, viewDir);
    }

    if (rightHeadlight.enabled) {
        lighting += CalcHeadlight(rightHeadlight, norm, fragWorldPos, viewDir);
    }
    if(flashLight.enabled) {
        lighting += CalcDirLight(flashLight, norm, viewDir);
	}

    vec3 I = normalize(viewDir);
    vec3 R = reflect(I, norm);
    vec3 reflection = texture(environmentMap, R).rgb;

    vec3 finalColor = lighting * rainColor.rgb + reflection * 0.3; // 0.3 is reflection strength

    FragColor = vec4(finalColor, rainColor.a * alpha);
}


vec3 CalcDirLight(DirLight light, vec3 norm, vec3 viewDir) {
    vec3 lightDir = normalize(-light.direction);
    vec3 ambient = light.ambient * light.color;

    float diff = max(dot(norm, lightDir), 0.0);
    vec3 diffuse = diff * light.diffuse * light.color;

    vec3 halfDir = normalize(lightDir + viewDir);
    float spec = pow(max(dot(norm, halfDir), 0.0), shininess);
    vec3 specular = light.specular * spec;
    return (ambient + diffuse + specular);
}

vec3 CalcPointLight(PointLight light, vec3 normal, vec3 fragPos, vec3 viewDir) {
    float distance = length(light.position - fragPos);
    float attenuation = 1.0 / (light.constant + light.linear * distance + light.quadratic * (distance * distance));
    vec3 ambient = light.ambient * light.color * attenuation;

    vec3 lightDir = normalize(light.position - fragPos);
    float diff = max(dot(normal, lightDir), 0.0);
    vec3 diffuse = light.diffuse * light.color * diff * attenuation;

    vec3 halfDir = normalize(lightDir + viewDir);
    float spec = pow(max(dot(normal, halfDir), 0.0), 32.0);
    vec3 specular = light.specular * spec * attenuation;
    return (ambient + diffuse + specular);
}

vec3 CalcSpotLight(SpotLight light, vec3 normal, vec3 fragPos, vec3 viewDir) {
    vec3 ambient = light.ambient * light.color;

    vec3 lightDir = normalize(light.position - fragPos);
    float diff = max(dot(normal, lightDir), 0.0);
    vec3 diffuse = light.diffuse * light.color * diff;

    vec3 halfDir = normalize(lightDir + viewDir);
    float spec = pow(max(dot(normal, halfDir), 0.0), 32.0);
    vec3 specular = light.specular * spec;
    float theta = dot(lightDir, normalize(-light.direction));
    float epsilon = light.cutOff - light.outerCutOff;
    float intensity = clamp((theta - light.outerCutOff) / epsilon, 0.0, 1.0);

    diffuse *= intensity;
    specular *= intensity;
    return (ambient + diffuse + specular);
}

vec3 CalcHeadlight(Headlight light, vec3 normal, vec3 fragPos, vec3 viewDir) {
	vec3 ambient = light.ambient * light.color;

	vec3 lightDir = normalize(light.position - fragPos);
	float diff = max(dot(normal, lightDir), 0.0);
	vec3 diffuse = light.diffuse * light.color * diff;

	vec3 halfDir = normalize(lightDir + viewDir);
	float spec = pow(max(dot(normal, halfDir), 0.0), 32.0);
	vec3 specular = light.specular * spec;
	float theta = dot(lightDir, normalize(-light.direction));
	float epsilon = light.cutOff - light.outerCutOff;
	float intensity = clamp((theta - light.outerCutOff) / epsilon, 0.0, 1.0);

	diffuse *= intensity;
	specular *= intensity;
	return (ambient + diffuse + specular);
}
