#version 330 core

in vec3 vNormal;

out vec4 FragColor;

void main() {
    vec3 normal = normalize(vNormal);
    vec3 lightDir = normalize(vec3(-0.5, -1.0, -0.3));
    float diffuse = max(dot(normal, -lightDir), 0.15);
    vec3 baseColor = vec3(0.20, 0.75, 0.85);
    FragColor = vec4(baseColor * diffuse, 1.0);
}
