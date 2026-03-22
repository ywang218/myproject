#version 330 core
out vec4 FragColor;

uniform float dashSize = 2.0;
uniform float gapSize = 2.0;

varying float vDist;
varying vec3 vColor;
varying float vDash;

void main() {
    float alpha = 1.0;
    if (vDash > 0.5) {
        float total = dashSize + gapSize;
        float m = mod(vDist, total);
        if (m > dashSize) discard;
    }
    FragColor = vec4(vColor, 1.0);
}