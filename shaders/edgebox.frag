#version 330 core
out vec4 FragColor;

uniform vec4 color; // 从 C++ shader->setVec4 传进来的值

void main() {
    // 直接输出颜色
    // 如果你想做点高级的，可以在这里判断：如果 alpha 极低，则 discard
    if (color.a < 0.05) {
        discard;
    }
    FragColor = color;
}