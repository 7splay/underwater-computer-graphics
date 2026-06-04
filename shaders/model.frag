#version 330 core

in vec3 vNormal;
in vec2 vTexCoord;

uniform sampler2D uTexture;

out vec4 FragColor;

void main() {
    vec3 normal = normalize(vNormal);
    vec3 lightDir = normalize(vec3(-0.5, -1.0, -0.3));
    float diffuse = max(dot(normal, -lightDir), 0.15);
    vec3 baseColor = texture(uTexture, vTexCoord).rgb;
    FragColor = vec4(baseColor * diffuse, 1.0);
}
