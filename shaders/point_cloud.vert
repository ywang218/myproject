#version 430 core
layout(location = 0) in vec4 aPos;
layout(location = 1) in vec4 aColor;

uniform mat4 uProjection;
uniform mat4 uView;
out vec4 vColor;

void main() {
    gl_Position = uProjection * uView * vec4(aPos.xyz, 1.0);
    vColor = aColor;
    gl_PointSize = 2.0; 
}