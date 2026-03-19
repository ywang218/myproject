#version 430 core

struct Point {
    vec4 pos;
    vec4 color;
};

layout(std430, binding = 0) buffer PointBuffer {
    Point points[];
};

uniform mat4 uProjection;
uniform mat4 uView;

void main() {
    // 确保这里的索引和计算是正确的
    vec4 worldPos = vec4(points[gl_VertexID].pos.xyz, 1.0);
    gl_Position = uProjection * uView * worldPos;
    
    // 极其重要：给点一个固定大小，否则它们可能只有 1 像素，你看不见
    gl_PointSize = 4.0; 
}