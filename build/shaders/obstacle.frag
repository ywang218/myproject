#version 330 core

in vec4 vColor;
out vec4 FragColor;

void main() {
    // 我们可以直接输出颜色
    // 如果你想让障碍物看起来有点透明度，可以使用 vColor
    FragColor = vColor;
    
    // 调试技巧：如果你还是看不见，可以先把下面这行取消注释，强制显示为红色
    // FragColor = vec4(1.0, 0.0, 0.0, 1.0); 
}