#version 330 core

in vec3 vNormal;
in vec2 vTexCoord;
in mat3 vTBN;

uniform sampler2D uTexture;
uniform sampler2D uNormalMap;
uniform bool uUseNormalMap;

out vec4 FragColor;

void main() {
    vec3 normal = normalize(vNormal);
    if (uUseNormalMap) {
        vec3 tangentNormal = texture(uNormalMap, vTexCoord).rgb;
        tangentNormal = tangentNormal * 2.0 - 1.0;
        normal = normalize(vTBN * tangentNormal);
    }

    vec3 lightDir = normalize(vec3(-0.5, -1.0, -0.3));
    float diffuse = max(dot(normal, -lightDir), 0.15);
    vec3 baseColor = texture(uTexture, vTexCoord).rgb;
    FragColor = vec4(baseColor * diffuse, 1.0);
}
