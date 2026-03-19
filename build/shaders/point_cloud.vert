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

out vec4 vColor;

void main() {
    vec4 worldPos = vec4(points[gl_VertexID].pos.xyz, 1.0);
    gl_Position = uProjection * uView * worldPos;
    
    vColor = points[gl_VertexID].color;
    gl_PointSize = 2.0; 
}