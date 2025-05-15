// text.vert
#version 330 core
layout (location = 0) in vec2 aPos;

uniform mat4 u_projection;

void main() {
    gl_Position = u_projection * vec4(aPos, 0.0, 1.0);
}