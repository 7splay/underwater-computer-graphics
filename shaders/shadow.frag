#version 330 core

// depth is written implicitly; the only job here is discarding transparent
// fragments of cutout meshes so they cast a correct silhouette instead of a
// solid quad. mirrors the alpha logic in model.frag
in vec2 vTexCoord;

uniform sampler2D uTexture;
uniform sampler2D uOpacityMap;
uniform float uAlphaCutoff;
// 0 = solid (no test), 1 = alpha channel, 2 = opacity map, 3 = luminance
uniform int uAlphaMode;

void main() {
    if (uAlphaMode == 1) {
        if (texture(uTexture, vTexCoord).a < uAlphaCutoff) discard;
    } else if (uAlphaMode == 2) {
        if (texture(uOpacityMap, vTexCoord).r < uAlphaCutoff) discard;
    } else if (uAlphaMode == 3) {
        vec4 t = texture(uTexture, vTexCoord);
        float alpha = max(max(t.r, t.g), t.b);
        if (t.a < 0.99) alpha = max(alpha, t.a);
        if (alpha < uAlphaCutoff) discard;
    }
}
