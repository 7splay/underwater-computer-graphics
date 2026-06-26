#version 330 core

// depth-only vertex shader for the shadow pass. uv is passed through so cutout
// meshes (seaweed, fish fins) can discard transparent fragments in shadow.frag
layout (location = 0) in vec3 aPos;
layout (location = 2) in vec2 aTexCoord;
layout (location = 4) in vec4 aInstRow0;
layout (location = 5) in vec4 aInstRow1;
layout (location = 6) in vec4 aInstRow2;
layout (location = 7) in vec4 aInstRow3;

uniform mat4 uModel;
uniform mat4 uLightSpace;  // flashlight projection * view
uniform int  uUseInstancing;

out vec2 vTexCoord;

void main() {
    mat4 model = (uUseInstancing == 1)
        ? mat4(aInstRow0, aInstRow1, aInstRow2, aInstRow3)
        : uModel;
    gl_Position = uLightSpace * model * vec4(aPos, 1.0);
    vTexCoord = aTexCoord;
}
