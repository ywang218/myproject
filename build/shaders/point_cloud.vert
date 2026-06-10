#version 430 core
layout(location = 0) in vec4 aPos;
layout(location = 1) in vec4 aColor;

uniform mat4 uProjection;
uniform mat4 uView;
uniform vec3 uEgoWorld; // 本帧实时车辆位置：把传感器相对坐标摆进世界
out vec4 vColor;

void main() {
    gl_Position = uProjection * uView * vec4(aPos.xyz + uEgoWorld, 1.0);
    vColor = aColor;
    gl_PointSize = 2.0;
}