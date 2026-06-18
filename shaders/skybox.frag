#version 330 core

in vec3 vDir;

out vec4 FragColor;

uniform samplerCube uSkybox;
uniform vec3 uSunDirection;
uniform vec3 uSunColor;

void main() {
    vec3 dir = normalize(vDir);
    vec3 base = texture(uSkybox, dir).rgb;

    // soft, dim underwater sun glow layered on top of the baked-in glow
    float cosAngle = max(dot(dir, normalize(uSunDirection)), 0.0);
    float halo = pow(cosAngle, 200.0) * 0.20;
    float bloom = pow(cosAngle, 6.0) * 0.05;

    vec3 color = base + uSunColor * (halo + bloom);
    FragColor = vec4(color, 1.0);
}
