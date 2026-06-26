#version 330 core

in vec3 vNormal;
in vec2 vTexCoord;
in mat3 vTBN;
in vec3 vWorldPos;
in vec4 vLightSpacePos;

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
// alpha cutoff for cutout-style flat meshes (fish fins, seaweed). fragments
// below this alpha are discarded so transparent regions don't render as blobs
uniform float uAlphaCutoff = 0.5f;
uniform bool uUseAlphaTest = false;
uniform vec3 uCameraPos;
uniform vec3 uFogColor;
uniform float uFogDensity;
uniform float uFogMax;
uniform vec3 uAmbientColor;

// flashlight shadow map + cone parameters
uniform sampler2DShadow uShadowMap;
uniform vec3  uFlashPos;        // helmet light world position
uniform vec3  uFlashDir;        // normalized beam direction
uniform float uFlashRange;      // max beam length
uniform float uFlashCosInner;   // cos(inner half angle) - full intensity
uniform float uFlashCosOuter;   // cos(outer half angle) - zero intensity
uniform vec3  uFlashColor;
uniform float uFlashIntensity;
uniform bool  uFlashEnabled;
uniform int   uShadowEnabled;   // 1 if the shadow map was rendered this frame
uniform sampler2D uOpacityMap;
uniform int uUseAlphaCutout;
uniform int uUseOpacityCutout;

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

// spotlight intensity at a world point: 0 outside the cone or range,
// soft falloff between the inner and outer angle
float spotlightIntensity(vec3 worldPos) {
    vec3 toP = worldPos - uFlashPos;
    float dist = length(toP);
    if (dist > uFlashRange) return 0.0;
    if (dist < 0.001) return 1.0;

    float cosA = dot(toP / dist, uFlashDir);
    if (cosA < uFlashCosOuter) return 0.0;

    float cone = (cosA - uFlashCosOuter) /
                 max(uFlashCosInner - uFlashCosOuter, 0.0001);
    cone = clamp(cone, 0.0, 1.0);
    cone *= cone;  // softer edge

    // fade the beam out before the hard range cutoff
    float distAttn = 1.0 - smoothstep(uFlashRange * 0.6, uFlashRange, dist);

    return cone * distAttn;
}

// 4-tap PCF using hardware depth compare. kept small (12 taps ran in every
// visible fragment and dominated the lit-pass cost; 4 still softens edges)
float shadowFactor(vec3 normal) {
    if (uShadowEnabled == 0) return 1.0;

    vec3 proj = vLightSpacePos.xyz / vLightSpacePos.w;
    if (proj.z > 1.0) return 1.0;
    if (proj.x < 0.0 || proj.x > 1.0 || proj.y < 0.0 || proj.y > 1.0) {
        return 1.0;
    }

    // slope-scaled bias to avoid acne
    vec3 L = normalize(uFlashPos - vWorldPos);
    float NdotL = max(dot(normalize(normal), L), 0.0);
    float bias = max(0.0015 * (1.0 - NdotL), 0.0004);

    vec2 texelSize = 1.0 / vec2(textureSize(uShadowMap, 0));
    float spread = 1.5 * texelSize.x;
    const vec2 offsets[4] = vec2[4](
        vec2(-0.5, -0.5), vec2( 0.5, -0.5),
        vec2(-0.5,  0.5), vec2( 0.5,  0.5)
    );
    float shadow = 0.0;
    for (int i = 0; i < 4; ++i) {
        vec2 uv = proj.xy + offsets[i] * spread;
        shadow += texture(uShadowMap, vec3(uv, proj.z - bias));
    }
    return shadow / 4.0;
}

void main() {
    // normal mapping: tangent-space perturbation via TBN from model.vert
    vec3 N = normalize(vNormal);
    if (uUseNormalMap) {
        vec3 tangentNormal = texture(uNormalMap, vTexCoord).rgb;
        tangentNormal = tangentNormal * 2.0 - 1.0;
        tangentNormal.xy *= uNormalStrength;
        tangentNormal = normalize(tangentNormal);
        N = normalize(vTBN * tangentNormal);
    }

    vec4 texSample = texture(uTexture, vTexCoord);

    // cutout-style meshes (fish, fins) bake the silhouette into the alpha channel
    // so transparent pixels must be discarded before any lighting
    if (uUseAlphaTest && texSample.a < uAlphaCutoff) {
        discard;
    }
    // seaweed-style cutouts use a separate opacity map or luminance fallback
    if (uUseAlphaCutout != 0) {
        float alpha;
        if (uUseOpacityCutout != 0) {
            alpha = texture(uOpacityMap, vTexCoord).r;
        } else {
            alpha = max(max(texSample.r, texSample.g), texSample.b);
            if (texSample.a < 0.99) {
                alpha = max(alpha, texSample.a);
            }
        }
        if (alpha < uAlphaCutoff) {
            discard;
        }
    }

    vec3 albedo = texSample.rgb * uAlbedoTint;

    // PBR metallic-roughness: per-pixel maps or material uniforms
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
    vec3 sunColor = vec3(0.45, 0.58, 0.70);
    vec3 fillColor = vec3(0.10, 0.20, 0.26);

    vec3 Lo = vec3(0.0);
    Lo += evaluatePbrLight(N, V, sunDir, sunColor, 0.55, albedo, metallic,
                           roughness, F0);
    Lo += evaluatePbrLight(N, V, fillDir, fillColor, 0.20, albedo, metallic,
                           roughness, F0);

    // flashlight: cone spotlight from the helmet, shadow factor darkens
    // occluded fragments while the cone test gives the soft beam
    if (uFlashEnabled) {
        float spot = spotlightIntensity(vWorldPos);
        // floor the shadow so it's never fully black (underwater scatter).
        // lower floor = darker shadow = more visible silhouette
        float shadow = mix(0.08, 1.0, shadowFactor(N));
        if (spot > 0.0) {
            vec3 L = normalize(uFlashPos - vWorldPos);
            Lo += evaluatePbrLight(N, V, L, uFlashColor,
                                   uFlashIntensity * spot * shadow, albedo,
                                   metallic, roughness, F0);
            // small water-tinted bounce so unlit surfaces in the cone read
            Lo += albedo * uFlashColor * spot * 0.18;
        }
    }

    vec3 color = uAmbientColor * albedo + Lo;

    // exponential underwater fog, density/color from underwater_atmosphere.hpp
    float dist = length(vWorldPos - uCameraPos);
    float fogAmount = 1.0 - exp(-dist * uFogDensity);
    color = mix(color, uFogColor, clamp(fogAmount, 0.0, uFogMax));

    FragColor = vec4(color, 1.0);
}
