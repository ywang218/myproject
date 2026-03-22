/**
 * // #include <glad/glad.h>
// #include <GLFW/glfw3.h>
// #include <glm/glm.hpp>
// #include <glm/gtc/matrix_transform.hpp>
// #include <glm/gtc/type_ptr.hpp>
// #include <iostream>
// #include <vector>
// #include <string>
// #include <thread>   // 新增：多线程支持
// #include <atomic>   // 新增：原子操作，用于安全退出
// #include "renderer/Shader.hpp"

// const int POINT_COUNT = 1000000;
// struct Point { alignas(16) float pos[4]; alignas(16) float color[4]; };

// // 全局共享变量
// std::atomic<bool> keepRunning(true);
// Point* sharedMappedPtr = nullptr; 

// // --- 线程 A: 数据生产者 (等同于你的 Web Worker) ---
// void DataProducerWorker() {
//     // 模拟后端解析逻辑 (如 FlatBuffers 解析或雷达原始数据处理)
//     while (keepRunning) {
//         // 这里模拟高负载计算，不需要强行同步到 60FPS
//         for (int i = 0; i < POINT_COUNT; i++) {
//             // 直接操作共享指针
//             sharedMappedPtr[i].pos[0] = 10.0f + (float)(rand() % 2000) / 40.0f; // r
//             sharedMappedPtr[i].pos[1] = (float)(rand() % 628) / 100.0f;        // theta
//             sharedMappedPtr[i].pos[2] = (float)(rand() % 100 - 50) / 100.0f;   // phi
//             sharedMappedPtr[i].color[0] = 0.5f + (float)(rand() % 50) / 100.0f; // Intensity
//         }
//         // 计算完一轮后可以稍微休息，或者立刻开始下一轮
//         std::this_thread::yield(); 
//     }
// }

// // 交互变量 (保持不变)
// float cameraDist = 60.0f;
// float yaw = 0.0f, pitch = 0.3f;
// double lastX, lastY;
// bool firstMouse = true, leftButtonPressed = false;

// // 回调函数 (略，与之前一致)
// void scroll_cb(GLFWwindow* w, double x, double y) { cameraDist -= (float)y * 2.0f; }
// void mouse_cb(GLFWwindow* w, double x, double y) {
//     if(firstMouse) { lastX = x; lastY = y; firstMouse = false; }
//     if(leftButtonPressed) {
//         yaw += (float)(x - lastX) * 0.005f; pitch += (float)(lastY - y) * 0.005f;
//         pitch = glm::clamp(pitch, -1.5f, 1.5f);
//     }
//     lastX = x; lastY = y;
// }
// void button_cb(GLFWwindow* w, int b, int a, int m) { if(b == GLFW_MOUSE_BUTTON_LEFT) leftButtonPressed = (a == GLFW_PRESS); }

// int main() {
//     glfwInit();
//     glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
//     glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
//     GLFWwindow* window = glfwCreateWindow(1280, 720, "C++ Decoupled Cloud Viewer", NULL, NULL);
//     glfwMakeContextCurrent(window);
//     glfwSetCursorPosCallback(window, mouse_cb);
//     glfwSetScrollCallback(window, scroll_cb);
//     glfwSetMouseButtonCallback(window, button_cb);
    
//     // 必须关闭 VSync 以观察纯粹的渲染性能
//     glfwSwapInterval(0);

//     gladLoadGLLoader((GLADloadproc)glfwGetProcAddress);
//     glEnable(GL_PROGRAM_POINT_SIZE);
//     glEnable(GL_DEPTH_TEST);

//     // --- 初始化 Persistent Mapping SSBO ---
//     GLuint ssbo;
//     glGenBuffers(1, &ssbo);
//     glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssbo);
    
//     GLbitfield flags = GL_MAP_WRITE_BIT | GL_MAP_PERSISTENT_BIT | GL_MAP_COHERENT_BIT;
//     glBufferStorage(GL_SHADER_STORAGE_BUFFER, POINT_COUNT * sizeof(Point), nullptr, flags);
//     sharedMappedPtr = (Point*)glMapBufferRange(GL_SHADER_STORAGE_BUFFER, 0, POINT_COUNT * sizeof(Point), flags);
//     glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, ssbo);

//     Shader renderShader("shaders/point_cloud.vert", "shaders/point_cloud.frag");
//     Shader computeShader("shaders/point_cloud.comp");
//     GLuint vao; glGenVertexArrays(1, &vao);

//     // --- 核心步骤：启动生产者线程 ---
//     std::thread producerThread(DataProducerWorker);

//     double lastTime = glfwGetTime();
//     int nbFrames = 0;

//     while (!glfwWindowShouldClose(window)) {
//         double currentTime = glfwGetTime();
//         nbFrames++;
//         if (currentTime - lastTime >= 1.0) {
//             glfwSetWindowTitle(window, ("Render FPS: " + std::to_string(nbFrames)).c_str());
//             nbFrames = 0; lastTime += 1.0;
//         }

//         // --- 渲染线程不等待生产者，只管画当前内存里的东西 ---
        
//         // 1. GPU 计算坐标转换
//         computeShader.use();
//         glUniform1f(glGetUniformLocation(computeShader.ID, "uTime"), (float)currentTime);
//         glDispatchCompute((POINT_COUNT + 255) / 256, 1, 1);
//         glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);

//         // 2. 绘制过程
//         glClearColor(0.01f, 0.01f, 0.03f, 1.0f);
//         glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

//         glm::vec3 camPos(cameraDist * cos(pitch) * cos(yaw), cameraDist * sin(pitch), cameraDist * cos(pitch) * sin(yaw));
//         glm::mat4 proj = glm::perspective(glm::radians(45.0f), 1280.0f/720.0f, 0.1f, 1000.0f);
//         glm::mat4 view = glm::lookAt(camPos, glm::vec3(0,0,0), glm::vec3(0,1,0));

//         renderShader.use();
//         glUniformMatrix4fv(glGetUniformLocation(renderShader.ID, "uProjection"), 1, GL_FALSE, glm::value_ptr(proj));
//         glUniformMatrix4fv(glGetUniformLocation(renderShader.ID, "uView"), 1, GL_FALSE, glm::value_ptr(view));

//         glBindVertexArray(vao);
//         glDrawArrays(GL_POINTS, 0, POINT_COUNT);

//         glfwSwapBuffers(window);
//         glfwPollEvents();
//     }

//     // 安全退出线程
//     keepRunning = false;
//     producerThread.join();

//     glfwTerminate();
//     return 0;
// }

 */

#include "renderer/PointCloudLayer.hpp"
#include <glm/gtc/type_ptr.hpp>

PointCloudLayer::PointCloudLayer(int count) : pointCount(count), keepRunning(true) {
    // 1. 加载 Shader
    renderShader = std::make_unique<Shader>("shaders/point_cloud.vert", "shaders/point_cloud.frag");
    computeShader = std::make_unique<Shader>("shaders/point_cloud.comp");

    // 2. 初始化 Persistent Mapping SSBO
    glGenBuffers(1, &ssbo);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssbo);
    
    GLbitfield flags = GL_MAP_WRITE_BIT | GL_MAP_PERSISTENT_BIT | GL_MAP_COHERENT_BIT;
    glBufferStorage(GL_SHADER_STORAGE_BUFFER, pointCount * sizeof(Point), nullptr, flags);
    sharedMappedPtr = (Point*)glMapBufferRange(GL_SHADER_STORAGE_BUFFER, 0, pointCount * sizeof(Point), flags);
    
    // 绑定到插槽 0
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, ssbo);

    // 3. 初始化 VAO (点云只需要一个空的 VAO)
    glGenVertexArrays(1, &vao);

    // 4. 启动生产者线程
    producerThread = std::thread(&PointCloudLayer::dataProducerWorker, this);
}

PointCloudLayer::~PointCloudLayer() {
    keepRunning = false;
    if (producerThread.joinable()) producerThread.join();
    
    glUnmapBuffer(GL_SHADER_STORAGE_BUFFER);
    glDeleteBuffers(1, &ssbo);
    glDeleteVertexArrays(1, &vao);
}

void PointCloudLayer::dataProducerWorker() {
    // while (keepRunning) {
        for (int i = 0; i < pointCount; i++) {
            sharedMappedPtr[i].pos[0] = 10.0f + (float)(rand() % 2000) / 40.0f;
            sharedMappedPtr[i].pos[1] = (float)(rand() % 628) / 100.0f;
            sharedMappedPtr[i].pos[2] = (float)(rand() % 100 - 50) / 100.0f;
            sharedMappedPtr[i].color[0] = 0.5f + (float)(rand() % 50) / 100.0f;
        }
        std::this_thread::yield(); 
    // }
}

void PointCloudLayer::update(float currentTime) {
    computeShader->use();
    glUniform1f(glGetUniformLocation(computeShader->ID, "uTime"), currentTime);
    glDispatchCompute((pointCount + 255) / 256, 1, 1);
    glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
}

void PointCloudLayer::render(const glm::mat4& view, const glm::mat4& projection) {
    renderShader->use();
    renderShader->setMat4("uView", view);
    renderShader->setMat4("uProjection", projection);

    glBindVertexArray(vao);
    glDrawArrays(GL_POINTS, 0, pointCount);
}