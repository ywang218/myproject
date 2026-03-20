#include "renderer/Renderer.hpp"

int main() {
    // 实例化 Renderer (1280x720)
    Renderer renderer(1280, 720);

    // 启动主循环
    renderer.run();

    return 0;
}