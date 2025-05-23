#version 450

layout(location = 0) in vec3 vPos;
layout(location = 1) in vec3 vColor;

layout(location = 0) out vec3 fragColor;

void main() {
    gl_Position = vec4(vPos, 1.0);
    fragColor = vColor;
}
