#version 300 es
in vec4 a_position;
in vec2 a_texCoord;
uniform mat4 uTexMatrix;
out vec2 v_texCoord;

void main() {
    gl_Position = a_position;
    v_texCoord = (uTexMatrix * vec4(a_texCoord, 0.0, 1.0)).xy;
}