#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aPrev;
layout (location = 2) in vec3 aNext;
layout (location = 3) in float aSide;
layout (location = 4) in float aLineDist;
layout (location = 5) in vec3 aColor;
layout (location = 6) in float aDash;
layout (location = 7) in float aWidth;

uniform mat4 view;
uniform mat4 projection;

varying float vDist;
varying vec3 vColor;
varying float vDash;

void main() {
    vDist = aLineDist;
    vColor = aColor;
    vDash = aDash;

    vec3 dir;
    if (distance(aPos, aPrev) < 0.0001) {
        dir = normalize(aNext - aPos);
    } else if (distance(aPos, aNext) < 0.0001) {
        dir = normalize(aPos - aPrev);
    } else {
        dir = normalize(normalize(aPos - aPrev) + normalize(aNext - aPos));
    }

    // 计算法线进行挤出 (假设是在 XY 平面上平铺)
    vec3 normal = vec3(-dir.y, dir.x, 0.0);
    if (length(normal) < 0.0001) normal = vec3(1.0, 0.0, 0.0);
    else normal = normalize(normal);

    vec3 newPos = aPos + normal * aSide * aWidth * 0.5;
    gl_Position = projection * view * vec4(newPos, 1.0);
}