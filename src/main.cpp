// #include <glad/glad.h>
// #include <GLFW/glfw3.h>
// #include <glm/glm.hpp>
// #include <glm/gtc/matrix_transform.hpp>
// #include <glm/gtc/type_ptr.hpp>
// #include <iostream>
// #include <algorithm>
// #include <string>    // 用于处理字符串
// #include "renderer/Shader.hpp"

// // 相机控制变量 (保持不变)
// float cameraDist = 30.0f;
// float yaw = 0.0f;
// float pitch = 0.0f;
// double lastX = 640.0, lastY = 360.0;
// bool firstMouse = true;
// bool leftMouseButtonPressed = false;

// // 窗口标题控制相关
// double lastTime = 0.0;
// int nbFrames = 0;

// void scroll_callback(GLFWwindow* window, double xoffset, double yoffset) {
//     cameraDist -= (float)yoffset * 2.0f;
//     cameraDist = std::max(1.0f, std::min(cameraDist, 200.0f));
// }

// void mouse_callback(GLFWwindow* window, double xpos, double ypos) {
//     if (firstMouse) { lastX = xpos; lastY = ypos; firstMouse = false; }
//     float xoffset = xpos - lastX;
//     float yoffset = lastY - ypos; 
//     lastX = xpos; lastY = ypos;
//     if (leftMouseButtonPressed) {
//         float sensitivity = 0.3f;
//         yaw += xoffset * sensitivity;
//         pitch += yoffset * sensitivity;
//         pitch = std::max(-89.0f, std::min(pitch, 89.0f));
//     }
// }

// void mouse_button_callback(GLFWwindow* window, int button, int action, int mods) {
//     if (button == GLFW_MOUSE_BUTTON_LEFT) {
//         if (action == GLFW_PRESS) leftMouseButtonPressed = true;
//         else if (action == GLFW_RELEASE) leftMouseButtonPressed = false;
//     }
// }

// struct Point {
//     alignas(16) float pos[4];
//     alignas(16) float color[4];
// };

// const int POINT_COUNT = 500000;

// int main() {
//     glfwInit();
//     glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
//     glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
//     GLFWwindow* window = glfwCreateWindow(1280, 720, "C++ Cloud Viewer", NULL, NULL);
//     glfwMakeContextCurrent(window);
    
//     glfwSetCursorPosCallback(window, mouse_callback);
//     glfwSetScrollCallback(window, scroll_callback);
//     glfwSetMouseButtonCallback(window, mouse_button_callback);

//     gladLoadGLLoader((GLADloadproc)glfwGetProcAddress);
//     glEnable(GL_PROGRAM_POINT_SIZE);
//     glEnable(GL_DEPTH_TEST);

//     // --- SSBO 初始化 ---
//     GLuint ssbo;
//     glGenBuffers(1, &ssbo);
//     glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssbo);
//     GLbitfield flags = GL_MAP_WRITE_BIT | GL_MAP_PERSISTENT_BIT | GL_MAP_COHERENT_BIT;
//     glBufferStorage(GL_SHADER_STORAGE_BUFFER, POINT_COUNT * sizeof(Point), nullptr, flags);
//     Point* mappedPtr = (Point*)glMapBufferRange(GL_SHADER_STORAGE_BUFFER, 0, POINT_COUNT * sizeof(Point), flags);

//     // 初始位置
//     for(int i = 0; i < POINT_COUNT; i++) {
//         mappedPtr[i].pos[0] = (rand() % 4000 - 2000) / 100.0f;
//         mappedPtr[i].pos[1] = (rand() % 4000 - 2000) / 100.0f;
//         mappedPtr[i].pos[2] = (rand() % 4000 - 2000) / 100.0f;
//         mappedPtr[i].pos[3] = 1.0f;
//         // 给个随机颜色
//         mappedPtr[i].color[0] = (rand() % 100) / 100.0f;
//         mappedPtr[i].color[1] = (rand() % 100) / 100.0f;
//         mappedPtr[i].color[2] = 1.0f; 
//         mappedPtr[i].color[3] = 1.0f;
//     }
//     glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, ssbo);

//     Shader pointShader("shaders/point_cloud.vert", "shaders/point_cloud.frag");
//     GLuint vao; glGenVertexArrays(1, &vao);

//     lastTime = glfwGetTime();

//     while (!glfwWindowShouldClose(window)) {
//         // 1. FPS 计算与显示
//         double currentTime = glfwGetTime();
//         nbFrames++;
//         if (currentTime - lastTime >= 1.0) { // 每秒更新一次
//             std::string title = "C++ Cloud Viewer | FPS: " + std::to_string(nbFrames) + 
//                                 " | Points: " + std::to_string(POINT_COUNT / 1000) + "K";
//             glfwSetWindowTitle(window, title.c_str());
//             nbFrames = 0;
//             lastTime += 1.0;
//         }

//         // 2. 随机运动逻辑 (直接操作显存指针)
//         // 模拟布朗运动，每一帧给点一个微小的偏移
//         for(int i = 0; i < POINT_COUNT; i++) {
//             mappedPtr[i].pos[0] += (rand() % 10 - 5) / 500.0f;
//             mappedPtr[i].pos[1] += (rand() % 10 - 5) / 500.0f;
//             mappedPtr[i].pos[2] += (rand() % 10 - 5) / 500.0f;
//         }

//         glClearColor(0.02f, 0.02f, 0.02f, 1.0f);
//         glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

//         // 相机矩阵计算
//         glm::vec3 cameraPos;
//         cameraPos.x = cameraDist * cos(glm::radians(pitch)) * cos(glm::radians(yaw));
//         cameraPos.y = cameraDist * sin(glm::radians(pitch));
//         cameraPos.z = cameraDist * cos(glm::radians(pitch)) * sin(glm::radians(yaw));

//         glm::mat4 projection = glm::perspective(glm::radians(45.0f), 1280.0f/720.0f, 0.1f, 1000.0f);
//         glm::mat4 view = glm::lookAt(cameraPos, glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f));

//         pointShader.use();
//         glUniformMatrix4fv(glGetUniformLocation(pointShader.ID, "uProjection"), 1, GL_FALSE, glm::value_ptr(projection));
//         glUniformMatrix4fv(glGetUniformLocation(pointShader.ID, "uView"), 1, GL_FALSE, glm::value_ptr(view));
        
//         glBindVertexArray(vao);
//         glDrawArrays(GL_POINTS, 0, POINT_COUNT);

//         glfwSwapBuffers(window);
//         glfwPollEvents();
//     }

//     glfwTerminate();
//     return 0;
// }


#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <iostream>
#include <string>
#include "renderer/Shader.hpp"

// 相机交互变量
float cameraDist = 40.0f;
float yaw = 0.0f, pitch = 0.0f;
double lastX = 640.0, lastY = 360.0;
bool firstMouse = true, leftButtonPressed = false;

// FPS 计数
double lastTime = 0.0;
int nbFrames = 0;

// 回调函数
void scroll_cb(GLFWwindow* w, double x, double y) { cameraDist -= (float)y * 2.0f; }
void mouse_cb(GLFWwindow* w, double x, double y) {
    if(firstMouse) { lastX = x; lastY = y; firstMouse = false; }
    if(leftButtonPressed) {
        yaw += (x - lastX) * 0.2f; pitch += (lastY - y) * 0.2f;
        pitch = glm::clamp(pitch, -89.0f, 89.0f);
    }
    lastX = x; lastY = y;
}
void button_cb(GLFWwindow* w, int b, int a, int m) { if(b == GLFW_MOUSE_BUTTON_LEFT) leftButtonPressed = (a == GLFW_PRESS); }

struct Point { alignas(16) float pos[4]; alignas(16) float color[4]; };
const int POINT_COUNT = 3000000;

int main() {
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    GLFWwindow* window = glfwCreateWindow(1280, 720, "Compute Shader Point Cloud", NULL, NULL);
    glfwMakeContextCurrent(window);
    glfwSetCursorPosCallback(window, mouse_cb);
    glfwSetScrollCallback(window, scroll_cb);
    glfwSetMouseButtonCallback(window, button_cb);

    gladLoadGLLoader((GLADloadproc)glfwGetProcAddress);
    glEnable(GL_PROGRAM_POINT_SIZE);
    glEnable(GL_DEPTH_TEST);

    // 1. 创建 SSBO
    GLuint ssbo;
    glGenBuffers(1, &ssbo);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssbo);
    glBufferData(GL_SHADER_STORAGE_BUFFER, POINT_COUNT * sizeof(Point), NULL, GL_DYNAMIC_DRAW);
    
    // 初始数据填充 (只做一次)
    Point* data = (Point*)glMapBuffer(GL_SHADER_STORAGE_BUFFER, GL_WRITE_ONLY);
    for(int i=0; i<POINT_COUNT; i++) {
        data[i].pos[0] = (rand()%4000-2000)/100.0f; data[i].pos[1] = (rand()%4000-2000)/100.0f;
        data[i].pos[2] = (rand()%4000-2000)/100.0f; data[i].pos[3] = 1.0f;
        data[i].color[0] = 0.2f; data[i].color[1] = 0.6f; data[i].color[2] = 1.0f; data[i].color[3] = 1.0f;
    }
    glUnmapBuffer(GL_SHADER_STORAGE_BUFFER);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, ssbo);

    // 2. 加载着色器
    Shader renderShader("shaders/point_cloud.vert", "shaders/point_cloud.frag");
    Shader computeShader("shaders/point_cloud.comp");
    GLuint vao; glGenVertexArrays(1, &vao);

    lastTime = glfwGetTime();

    while (!glfwWindowShouldClose(window)) {
        // FPS 统计
        double currentTime = glfwGetTime();
        nbFrames++;
        if (currentTime - lastTime >= 1.0) {
            glfwSetWindowTitle(window, ("FPS: " + std::to_string(nbFrames)).c_str());
            nbFrames = 0; lastTime += 1.0;
        }

        // --- A. 计算阶段 ---
        computeShader.use();
        glUniform1f(glGetUniformLocation(computeShader.ID, "uTime"), (float)currentTime);
        glDispatchCompute((POINT_COUNT + 255) / 256, 1, 1);
        glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT); // 确保计算完成

        // --- B. 渲染阶段 ---
        glClearColor(0.05f, 0.05f, 0.05f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glm::vec3 camPos(
            cameraDist * cos(glm::radians(pitch)) * cos(glm::radians(yaw)),
            cameraDist * sin(glm::radians(pitch)),
            cameraDist * cos(glm::radians(pitch)) * sin(glm::radians(yaw))
        );
        glm::mat4 proj = glm::perspective(glm::radians(45.0f), 1280.0f/720.0f, 0.1f, 1000.0f);
        glm::mat4 view = glm::lookAt(camPos, glm::vec3(0,0,0), glm::vec3(0,1,0));

        renderShader.use();
        glUniformMatrix4fv(glGetUniformLocation(renderShader.ID, "uProjection"), 1, GL_FALSE, glm::value_ptr(proj));
        glUniformMatrix4fv(glGetUniformLocation(renderShader.ID, "uView"), 1, GL_FALSE, glm::value_ptr(view));

        glBindVertexArray(vao);
        glDrawArrays(GL_POINTS, 0, POINT_COUNT);

        glfwSwapBuffers(window);
        glfwPollEvents();
    }
    glfwTerminate();
    return 0;
}
