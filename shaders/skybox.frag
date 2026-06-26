#version 330 core

in vec3 vDir;

out vec4 FragColor;

uniform samplerCube uSkybox;
uniform vec3 uSunDirection;
uniform vec3 uSunColor;
uniform vec3 uFogColor;

void main() {
    vec3 dir = normalize(vDir);
    vec3 base = texture(uSkybox, dir).rgb;

    // soft sun glow layered on top of the baked-in glow
    float cosAngle = max(dot(dir, normalize(uSunDirection)), 0.0);
    float halo = pow(cosAngle, 200.0) * 0.20;
    float bloom = pow(cosAngle, 6.0) * 0.05;
    vec3 color = base + uSunColor * (halo + bloom);

    // skybox fog: blend toward the water colour everywhere except when
    // looking up. this kills the "inside a box" feel by hiding the skybox
    // walls and ceiling; a wider clear cone (pow 2) keeps the sun and the
    // surrounding surface glow readable
    float upness = clamp(dir.y, 0.0, 1.0);
    float skyFog = 1.0 - pow(upness, 2.0);
    color = mix(color, uFogColor, skyFog);

    FragColor = vec4(color, 1.0);
}
