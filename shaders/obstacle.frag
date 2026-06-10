#version 330 core

in vec4 vColor;
out vec4 FragColor;

uniform float uAlpha; // 由各图层设置：填充层 0.6，边框层 1.0

void main() {
    FragColor = vec4(vColor.rgb, vColor.a * uAlpha);
}