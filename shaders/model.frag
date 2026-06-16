#version 330 core

in vec3 vNormal;
in vec2 vTexCoord;
in mat3 vTBN;
in vec3 vWorldPos;

uniform sampler2D uTexture;
uniform sampler2D uNormalMap;
uniform sampler2D uRoughnessMap;
uniform sampler2D uMetallicMap;
uniform bool uUseNormalMap;
uniform bool uUseRoughnessMap;
uniform bool uUseMetallicMap;
uniform bool uUseArmMap;
uniform float uNormalStrength;
uniform float uMetallic;
uniform float uRoughness;
uniform vec3 uAlbedoTint;
uniform vec3 uCameraPos;
uniform vec3 uFogColor;
uniform float uFogDensity;
uniform float uFogMax;
uniform vec3 uAmbientColor;

out vec4 FragColor;

const float PI = 3.14159265359;

float DistributionGGX(vec3 N, vec3 H, float roughness) {
    float a = roughness * roughness;
    float a2 = a * a;
    float NdotH = max(dot(N, H), 0.0);
    float NdotH2 = NdotH * NdotH;
    float denom = (NdotH2 * (a2 - 1.0) + 1.0);
    denom = PI * denom * denom;
    return a2 / max(denom, 0.0001);
}

float GeometrySchlickGGX(float NdotV, float roughness) {
    float r = roughness + 1.0;
    float k = (r * r) / 8.0;
    return NdotV / (NdotV * (1.0 - k) + k);
}

float GeometrySmith(vec3 N, vec3 V, vec3 L, float roughness) {
    float NdotV = max(dot(N, V), 0.0);
    float NdotL = max(dot(N, L), 0.0);
    float ggx1 = GeometrySchlickGGX(NdotV, roughness);
    float ggx2 = GeometrySchlickGGX(NdotL, roughness);
    return ggx1 * ggx2;
}

vec3 FresnelSchlick(float cosTheta, vec3 F0) {
    return F0 + (1.0 - F0) * pow(clamp(1.0 - cosTheta, 0.0, 1.0), 5.0);
}

vec3 evaluatePbrLight(vec3 N, vec3 V, vec3 L, vec3 lightColor, float intensity,
                      vec3 albedo, float metallic, float roughness, vec3 F0) {
    vec3 H = normalize(V + L);
    float NdotL = max(dot(N, L), 0.0);
    if (NdotL <= 0.0) {
        return vec3(0.0);
    }

    float NDF = DistributionGGX(N, H, roughness);
    float G = GeometrySmith(N, V, L, roughness);
    vec3 F = FresnelSchlick(max(dot(H, V), 0.0), F0);

    vec3 numerator = NDF * G * F;
    float denominator = 4.0 * max(dot(N, V), 0.0) * NdotL + 0.0001;
    vec3 specular = numerator / denominator;

    vec3 kS = F;
    vec3 kD = vec3(1.0) - kS;
    kD *= 1.0 - metallic;

    return (kD * albedo / PI + specular) * NdotL * lightColor * intensity;
}

void main() {
    // Normal mapping: tangent-space perturbation via TBN from model.vert.
    vec3 N = normalize(vNormal);
    if (uUseNormalMap) {
        vec3 tangentNormal = texture(uNormalMap, vTexCoord).rgb;
        tangentNormal = tangentNormal * 2.0 - 1.0;
        tangentNormal.xy *= uNormalStrength;
        tangentNormal = normalize(tangentNormal);
        N = normalize(vTBN * tangentNormal);
    }

    vec3 albedo = texture(uTexture, vTexCoord).rgb * uAlbedoTint;

    // PBR metallic-roughness: per-pixel maps or material uniforms.
    float metallic = uMetallic;
    float roughness = uRoughness;
    if (uUseArmMap) {
        vec3 arm = texture(uRoughnessMap, vTexCoord).rgb;
        roughness = arm.g;
        metallic = arm.b;
    } else {
        if (uUseRoughnessMap) {
            roughness = texture(uRoughnessMap, vTexCoord).r;
        }
        if (uUseMetallicMap) {
            metallic = texture(uMetallicMap, vTexCoord).r;
        }
    }
    roughness = clamp(roughness, 0.04, 1.0);
    metallic = clamp(metallic, 0.0, 1.0);

    vec3 V = normalize(uCameraPos - vWorldPos);
    vec3 F0 = mix(vec3(0.04), albedo, metallic);

    vec3 sunDir = normalize(vec3(0.12, 1.0, 0.18));
    vec3 fillDir = normalize(vec3(-0.35, -0.25, -0.5));
    vec3 sunColor = vec3(0.72, 0.86, 0.95);
    vec3 fillColor = vec3(0.18, 0.34, 0.42);

    vec3 Lo = vec3(0.0);
    Lo += evaluatePbrLight(N, V, sunDir, sunColor, 1.15, albedo, metallic,
                           roughness, F0);
    Lo += evaluatePbrLight(N, V, fillDir, fillColor, 0.35, albedo, metallic,
                           roughness, F0);

    vec3 color = uAmbientColor * albedo + Lo;

    // Exponential underwater fog — density/color from underwater_atmosphere.hpp.
    float dist = length(vWorldPos - uCameraPos);
    float fogAmount = 1.0 - exp(-dist * uFogDensity);
    color = mix(color, uFogColor, clamp(fogAmount, 0.0, uFogMax));

    FragColor = vec4(color, 1.0);
}
