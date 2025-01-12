#version 410 core

// Vertex Attributes
layout(location = 0) in vec3 vPosition;
layout(location = 1) in vec3 vNormal;
layout(location = 2) in vec2 vTexCoords;
layout(location = 3) in vec3 vTangent;
layout(location = 4) in vec3 vBitangent;

// Outputs to Fragment Shader
out vec3 fPosition;
out vec3 fNormal;
out vec2 fTexCoords;
out mat3 TBN;
out vec4 FragPosLightSpace;
out vec4 FragPosLightSpaceLeftHeadlight;
out vec4 FragPosLightSpaceRightHeadlight;

// Uniforms for Transformations
uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;
uniform mat4 lightSpaceMatrix;
uniform mat4 leftHeadlightLightSpaceMatrix;
uniform mat4 rightHeadlightLightSpaceMatrix;

// Uniforms for Wind Simulation
uniform float u_Time;
uniform vec3 u_WindDirection;
uniform float u_WindStrength;
uniform float u_GustSize;
uniform float u_GustSpeed;
uniform float u_WindWaveLength;
uniform int u_ObjectType;
uniform int isWindMovable;
uniform int windEnabled;

// Uniform for Clipping Plane
uniform vec4 plane;

vec4 permute(vec4 x){
	return mod(((x*34.0)+1.0)*x, 289.0);
}
vec4 taylorInvSqrt(vec4 r){
	return 1.79284291400159 - 0.85373472095314 * r;
}
vec3 fade(vec3 t) {
	return t*t*t*(t*(t*6.0-15.0)+10.0);
}
float Perlin3DNoise(vec3 P){
	vec3 Pi0 = floor(P); // Integer part for indexing
	vec3 Pi1 = Pi0 + vec3(1.0); // Integer part + 1
	Pi0 = mod(Pi0, 289.0);
	Pi1 = mod(Pi1, 289.0);
	vec3 Pf0 = fract(P); // Fractional part for interpolation
	vec3 Pf1 = Pf0 - vec3(1.0); // Fractional part - 1.0
	vec4 ix = vec4(Pi0.x, Pi1.x, Pi0.x, Pi1.x);
	vec4 iy = vec4(Pi0.yy, Pi1.yy);
	vec4 iz0 = Pi0.zzzz;
	vec4 iz1 = Pi1.zzzz;

	vec4 ixy = permute(permute(ix) + iy);
	vec4 ixy0 = permute(ixy + iz0);
	vec4 ixy1 = permute(ixy + iz1);

	vec4 gx0 = ixy0 / 7.0;
	vec4 gy0 = fract(floor(gx0) / 7.0) - 0.5;
	gx0 = fract(gx0);
	vec4 gz0 = vec4(0.5) - abs(gx0) - abs(gy0);
	vec4 sz0 = step(gz0, vec4(0.0));
	gx0 -= sz0 * (step(0.0, gx0) - 0.5);
	gy0 -= sz0 * (step(0.0, gy0) - 0.5);

	vec4 gx1 = ixy1 / 7.0;
	vec4 gy1 = fract(floor(gx1) / 7.0) - 0.5;
	gx1 = fract(gx1);
	vec4 gz1 = vec4(0.5) - abs(gx1) - abs(gy1);
	vec4 sz1 = step(gz1, vec4(0.0));
	gx1 -= sz1 * (step(0.0, gx1) - 0.5);
	gy1 -= sz1 * (step(0.0, gy1) - 0.5);

	vec3 g000 = vec3(gx0.x,gy0.x,gz0.x);
	vec3 g100 = vec3(gx0.y,gy0.y,gz0.y);
	vec3 g010 = vec3(gx0.z,gy0.z,gz0.z);
	vec3 g110 = vec3(gx0.w,gy0.w,gz0.w);
	vec3 g001 = vec3(gx1.x,gy1.x,gz1.x);
	vec3 g101 = vec3(gx1.y,gy1.y,gz1.y);
	vec3 g011 = vec3(gx1.z,gy1.z,gz1.z);
	vec3 g111 = vec3(gx1.w,gy1.w,gz1.w);

	vec4 norm0 = taylorInvSqrt(vec4(dot(g000, g000), dot(g010, g010), dot(g100, g100), dot(g110, g110)));
	g000 *= norm0.x;
	g010 *= norm0.y;
	g100 *= norm0.z;
	g110 *= norm0.w;
	vec4 norm1 = taylorInvSqrt(vec4(dot(g001, g001), dot(g011, g011), dot(g101, g101), dot(g111, g111)));
	g001 *= norm1.x;
	g011 *= norm1.y;
	g101 *= norm1.z;
	g111 *= norm1.w;

	float n000 = dot(g000, Pf0);
	float n100 = dot(g100, vec3(Pf1.x, Pf0.yz));
	float n010 = dot(g010, vec3(Pf0.x, Pf1.y, Pf0.z));
	float n110 = dot(g110, vec3(Pf1.xy, Pf0.z));
	float n001 = dot(g001, vec3(Pf0.xy, Pf1.z));
	float n101 = dot(g101, vec3(Pf1.x, Pf0.y, Pf1.z));
	float n011 = dot(g011, vec3(Pf0.x, Pf1.yz));
	float n111 = dot(g111, Pf1);

	vec3 fade_xyz = fade(Pf0);
	vec4 n_z = mix(vec4(n000, n100, n010, n110), vec4(n001, n101, n011, n111), fade_xyz.z);
	vec2 n_yz = mix(n_z.xy, n_z.zw, fade_xyz.y);
	float n_xyz = mix(n_yz.x, n_yz.y, fade_xyz.x); 
	return 2.2 * n_xyz;
}

float getWindMultiplier(int objectType, float height, float minHeight, float maxHeight) {
    float heightFactor = clamp((height - minHeight) / (maxHeight - minHeight), 0.0, 1.0);
    if (objectType == 0) { // Grass and Stems
        return 0.3 * heightFactor;
    } else if (objectType == 2) { // Ferns
        return 0.6;
    } else { // Leaves and Default
        return 1.0;
    }
}

void main()
{
    // Start with the original position
    vec3 pos = vPosition;

    if (windEnabled == 1 && isWindMovable == 1) {
        // Determine wind multiplier based on object type

        float minHeight = -1.0;
        float maxHeight = 1.0;
                float windMultiplier = getWindMultiplier(u_ObjectType, vPosition.y, minHeight, maxHeight);

        vec3 windDir = normalize(u_WindDirection) * windMultiplier;

        float windFactor = (pos.x + pos.y + pos.z) / u_WindWaveLength + u_Time;

        float noise = Perlin3DNoise(vec3(
            pos.x / u_GustSize,
            pos.z / u_GustSize,
            u_Time * u_GustSpeed
        ));

        // Large Wind Power
        float largeWindPower = sin(windFactor) * windMultiplier;
        if (largeWindPower < 0.0) {
            largeWindPower *= 0.4;
        } else {
            largeWindPower *= 0.6;
        }
        largeWindPower *= noise;
        pos.x += largeWindPower * windDir.x;
        pos.z += largeWindPower * windDir.z;

        // Medium Wind Power
        float x = (2.0 * sin(1.0 * windFactor)) + 1.0;
        float z = (1.0 * sin(1.8 * windFactor)) + 0.5;
        vec3 mediumWindPower = vec3(x, 0.0, z) * vec3(0.1) * noise * windMultiplier;
        pos += mediumWindPower;

        // Small Wind Power
        float smallWindPower = 0.065 * sin(2.650 * windFactor);
        smallWindPower *= u_WindStrength * windMultiplier;

        vec3 smallJitter = vec3(smallWindPower);
        smallJitter *= vNormal;
        smallJitter *= vec3(1.0, 0.35, 1.0);
        smallJitter *= 0.075;
        smallJitter *= noise;
        pos += smallJitter;
    }

    // **Transform to World Space**
    vec4 worldPos = model * vec4(pos, 1.0);
    
    // **Final Position Calculation**
    gl_Position = projection * view * worldPos;

    // **Clipping Plane (Optional)**
    gl_ClipDistance[0] = dot(worldPos, plane);

    // **Pass Data to Fragment Shader**
    fPosition = vec3(worldPos);
    fNormal = normalize(mat3(transpose(inverse(model))) * vNormal);
    fTexCoords = vTexCoords;

    // **Tangent-Bitangent-Normal Matrix for Normal Mapping**
    vec3 T = normalize(mat3(model) * vTangent);
    vec3 B = normalize(mat3(model) * vBitangent);
    vec3 N = normalize(mat3(model) * vNormal);
    TBN = mat3(T, B, N);

    // **Light Space Positions for Shadow Mapping**
    FragPosLightSpace = lightSpaceMatrix * vec4(fPosition, 1.0);
    FragPosLightSpaceLeftHeadlight = leftHeadlightLightSpaceMatrix * vec4(fPosition, 1.0);
    FragPosLightSpaceRightHeadlight = rightHeadlightLightSpaceMatrix * vec4(fPosition, 1.0);
}
