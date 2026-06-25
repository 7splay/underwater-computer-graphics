#version 330 core

layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec2 aTexCoord;
layout (location = 3) in vec4 aTangent;
layout (location = 4) in vec4 aInstRow0;
layout (location = 5) in vec4 aInstRow1;
layout (location = 6) in vec4 aInstRow2;
layout (location = 7) in vec4 aInstRow3;

uniform mat4 uModel;
uniform mat4 uView;
uniform mat4 uProjection;
uniform mat4 uLightSpace;  // flashlight projection * view, for shadow lookup
uniform int uUseInstancing;
uniform int uBillboard;
uniform vec3 uCameraPos;

out vec3 vNormal;
out vec2 vTexCoord;
out mat3 vTBN;
out vec3 vWorldPos;
out vec4 vLightSpacePos;  // world pos in light clip space

void main() {
    mat4 model = (uUseInstancing == 1)
        ? mat4(aInstRow0, aInstRow1, aInstRow2, aInstRow3)
        : uModel;

    vec4 worldPos;
    if (uBillboard == 1) {
        vec3 center = vec3(model[3]);
        vec3 right = normalize(vec3(uView[0][0], uView[1][0], uView[2][0]));
        vec3 up = normalize(vec3(uView[0][1], uView[1][1], uView[2][1]));
        float width = length(vec3(model[0]));
        float height = length(vec3(model[1]));
        vec3 offset = right * (aPos.x * width) + up * (aPos.y * height);
        worldPos = vec4(center + offset, 1.0);
        vNormal = normalize(uCameraPos - center);
        vTBN = mat3(1.0);
    } else {
        worldPos = model * vec4(aPos, 1.0);
        mat3 normalMatrix = mat3(transpose(inverse(model)));
        vec3 T = normalize(normalMatrix * aTangent.xyz);
        vec3 N = normalize(normalMatrix * aNormal);
        T = normalize(T - dot(T, N) * N);
        vec3 B = cross(N, T) * aTangent.w;
        vTBN = mat3(T, B, N);
        vNormal = N;
    }

    vWorldPos = worldPos.xyz;
    vLightSpacePos = uLightSpace * worldPos;
    gl_Position = uProjection * uView * worldPos;
    vTexCoord = aTexCoord;
}
