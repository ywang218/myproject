#version 330 core

// 0: 基础盒子的顶点 (每个盒子都一样)
layout (location = 0) in vec3 aPos;

// 1-4: 实例化矩阵 (mat4 自动拆分为 4 个 vec4)
// 对应 C++ 中 glVertexAttribPointer 的 location 1, 2, 3, 4
layout (location = 1) in mat4 aInstanceMatrix;

// 5: 实例化颜色
// 对应 C++ 中 location 5
layout (location = 5) in vec4 aInstanceColor;

uniform mat4 view;
uniform mat4 projection;

out vec4 vColor;

void main() {
    vColor = aInstanceColor;
    
    // 计算最终位置: Projection * View * Model * LocalPos
    // 注意: aInstanceMatrix 就是每个障碍物的 Model 矩阵
    gl_Position = projection * view * aInstanceMatrix * vec4(aPos, 1.0);
}