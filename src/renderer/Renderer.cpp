#include "renderer/Renderer.hpp"
#include "renderer/PointCloudLayer.hpp"
#include <iostream>

// 全局/静态变量用于回调
static Camera* g_camera = nullptr;
static bool leftButtonPressed = false;
static double lastX, lastY;
static bool firstMouse = true;

// 简单输入回调实现
void mouse_callback(GLFWwindow* window, double xpos, double ypos) {
    if (firstMouse) { lastX = xpos; lastY = ypos; firstMouse = false; }
    if (leftButtonPressed && g_camera) {
        // 这里可以实现类似你 CameraController.js 的旋转逻辑
    }
    lastX = xpos; lastY = ypos;
}

Renderer::Renderer(int w, int h) : width(w), height(h) {
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    window = glfwCreateWindow(width, height, "AutoDrive Engine C++", NULL, NULL);
    if (!window) { glfwTerminate(); }
    
    glfwMakeContextCurrent(window);
    gladLoadGLLoader((GLADloadproc)glfwGetProcAddress);

    glEnable(GL_DEPTH_TEST);
    glEnable(GL_PROGRAM_POINT_SIZE);
    glfwSwapInterval(0); // 关闭垂直同步

    // 初始化相机和图层
    camera = std::make_unique<Camera>(glm::vec3(0, 30, 60));
    g_camera = camera.get(); // 供回调使用

    pointCloud = std::make_unique<PointCloudLayer>(1000000);
}

Renderer::~Renderer() {
    glfwTerminate();
}

void Renderer::run() {
    while (!glfwWindowShouldClose(window)) {
        float currentTime = (float)glfwGetTime();

        // 1. Update (数据处理)
        pointCloud->update(currentTime);

        // 2. Render (绘制)
        glClearColor(0.01f, 0.01f, 0.03f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glm::mat4 view = camera->GetViewMatrix();
        glm::mat4 proj = camera->GetProjectionMatrix((float)width, (float)height);

        pointCloud->render(view, proj);

        glfwSwapBuffers(window);
        glfwPollEvents();
    }
}