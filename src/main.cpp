// #include <glad/glad.h>
// #include <GLFW/glfw3.h>
// #include <glm/glm.hpp>
// #include <glm/gtc/matrix_transform.hpp>
// #include <glm/gtc/type_ptr.hpp>
// #include <iostream>
// #include <algorithm> // 用于 std::clamp
// #include "renderer/Shader.hpp"

// // 相机控制全局变量
// float cameraDist = 15.0f;
// float yaw = 0.0f;
// float pitch = 0.0f;
// double lastX = 640.0, lastY = 360.0;
// bool firstMouse = true;
// bool leftMouseButtonPressed = false;

// // 1. 鼠标滚动回调：实现缩放 (Zoom)
// void scroll_callback(GLFWwindow* window, double xoffset, double yoffset) {
//     cameraDist -= (float)yoffset * 1.0f; // 调整缩放灵敏度
//     if (cameraDist < 1.0f) cameraDist = 1.0f;
//     if (cameraDist > 100.0f) cameraDist = 100.0f;
// }

// // 2. 鼠标移动回调：实现旋转 (Rotate)
// void mouse_callback(GLFWwindow* window, double xpos, double ypos) {
//     if (firstMouse) {
//         lastX = xpos;
//         lastY = ypos;
//         firstMouse = false;
//     }

//     float xoffset = xpos - lastX;
//     float yoffset = lastY - ypos; // 注意 Y 是相反的
//     lastX = xpos;
//     lastY = ypos;

//     if (leftMouseButtonPressed) {
//         float sensitivity = 0.5f;
//         yaw   += xoffset * sensitivity;
//         pitch += yoffset * sensitivity;

//         // 限制俯仰角，防止相机翻转
//         if (pitch > 89.0f) pitch = 89.0f;
//         if (pitch < -89.0f) pitch = -89.0f;
//     }
// }

// // 3. 鼠标按键回调：只有按下左键才旋转
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
    
//     // 注册回调函数
//     glfwSetCursorPosCallback(window, mouse_callback);
//     glfwSetScrollCallback(window, scroll_callback);
//     glfwSetMouseButtonCallback(window, mouse_button_callback);

//     gladLoadGLLoader((GLADloadproc)glfwGetProcAddress);
//     glEnable(GL_PROGRAM_POINT_SIZE);
//     glEnable(GL_DEPTH_TEST);

//     // --- SSBO 初始化 (保持不变) ---
//     GLuint ssbo;
//     glGenBuffers(1, &ssbo);
//     glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssbo);
//     GLbitfield flags = GL_MAP_WRITE_BIT | GL_MAP_PERSISTENT_BIT | GL_MAP_COHERENT_BIT;
//     glBufferStorage(GL_SHADER_STORAGE_BUFFER, POINT_COUNT * sizeof(Point), nullptr, flags);
//     Point* mappedPtr = (Point*)glMapBufferRange(GL_SHADER_STORAGE_BUFFER, 0, POINT_COUNT * sizeof(Point), flags);

//     for(int i = 0; i < POINT_COUNT; i++) {
//         mappedPtr[i].pos[0] = (rand() % 4000 - 2000) / 100.0f;
//         mappedPtr[i].pos[1] = (rand() % 4000 - 2000) / 100.0f;
//         mappedPtr[i].pos[2] = (rand() % 4000 - 2000) / 100.0f;
//         mappedPtr[i].pos[3] = 1.0f;
//     }
//     glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, ssbo);

//     Shader pointShader("shaders/point_cloud.vert", "shaders/point_cloud.frag");
//     GLuint vao; glGenVertexArrays(1, &vao);

//     while (!glfwWindowShouldClose(window)) {
//         glClearColor(0.05f, 0.05f, 0.05f, 1.0f);
//         glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

//         // A. 计算相机矩阵
//         // 将球形坐标转换为 3D 直角坐标
//         glm::vec3 cameraPos;
//         cameraPos.x = cameraDist * cos(glm::radians(pitch)) * cos(glm::radians(yaw));
//         cameraPos.y = cameraDist * sin(glm::radians(pitch));
//         cameraPos.z = cameraDist * cos(glm::radians(pitch)) * sin(glm::radians(yaw));

//         glm::mat4 projection = glm::perspective(glm::radians(45.0f), 1280.0f/720.0f, 0.1f, 1000.0f);
//         glm::mat4 view = glm::lookAt(cameraPos, glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f));

//         // B. 渲染
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
#include <algorithm>
#include <string>    // 用于处理字符串
#include "renderer/Shader.hpp"

// 相机控制变量 (保持不变)
float cameraDist = 30.0f;
float yaw = 0.0f;
float pitch = 0.0f;
double lastX = 640.0, lastY = 360.0;
bool firstMouse = true;
bool leftMouseButtonPressed = false;

// 窗口标题控制相关
double lastTime = 0.0;
int nbFrames = 0;

void scroll_callback(GLFWwindow* window, double xoffset, double yoffset) {
    cameraDist -= (float)yoffset * 2.0f;
    cameraDist = std::max(1.0f, std::min(cameraDist, 200.0f));
}

void mouse_callback(GLFWwindow* window, double xpos, double ypos) {
    if (firstMouse) { lastX = xpos; lastY = ypos; firstMouse = false; }
    float xoffset = xpos - lastX;
    float yoffset = lastY - ypos; 
    lastX = xpos; lastY = ypos;
    if (leftMouseButtonPressed) {
        float sensitivity = 0.3f;
        yaw += xoffset * sensitivity;
        pitch += yoffset * sensitivity;
        pitch = std::max(-89.0f, std::min(pitch, 89.0f));
    }
}

void mouse_button_callback(GLFWwindow* window, int button, int action, int mods) {
    if (button == GLFW_MOUSE_BUTTON_LEFT) {
        if (action == GLFW_PRESS) leftMouseButtonPressed = true;
        else if (action == GLFW_RELEASE) leftMouseButtonPressed = false;
    }
}

struct Point {
    alignas(16) float pos[4];
    alignas(16) float color[4];
};

const int POINT_COUNT = 500000;

int main() {
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    GLFWwindow* window = glfwCreateWindow(1280, 720, "C++ Cloud Viewer", NULL, NULL);
    glfwMakeContextCurrent(window);
    
    glfwSetCursorPosCallback(window, mouse_callback);
    glfwSetScrollCallback(window, scroll_callback);
    glfwSetMouseButtonCallback(window, mouse_button_callback);

    gladLoadGLLoader((GLADloadproc)glfwGetProcAddress);
    glEnable(GL_PROGRAM_POINT_SIZE);
    glEnable(GL_DEPTH_TEST);

    // --- SSBO 初始化 ---
    GLuint ssbo;
    glGenBuffers(1, &ssbo);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssbo);
    GLbitfield flags = GL_MAP_WRITE_BIT | GL_MAP_PERSISTENT_BIT | GL_MAP_COHERENT_BIT;
    glBufferStorage(GL_SHADER_STORAGE_BUFFER, POINT_COUNT * sizeof(Point), nullptr, flags);
    Point* mappedPtr = (Point*)glMapBufferRange(GL_SHADER_STORAGE_BUFFER, 0, POINT_COUNT * sizeof(Point), flags);

    // 初始位置
    for(int i = 0; i < POINT_COUNT; i++) {
        mappedPtr[i].pos[0] = (rand() % 4000 - 2000) / 100.0f;
        mappedPtr[i].pos[1] = (rand() % 4000 - 2000) / 100.0f;
        mappedPtr[i].pos[2] = (rand() % 4000 - 2000) / 100.0f;
        mappedPtr[i].pos[3] = 1.0f;
        // 给个随机颜色
        mappedPtr[i].color[0] = (rand() % 100) / 100.0f;
        mappedPtr[i].color[1] = (rand() % 100) / 100.0f;
        mappedPtr[i].color[2] = 1.0f; 
        mappedPtr[i].color[3] = 1.0f;
    }
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, ssbo);

    Shader pointShader("shaders/point_cloud.vert", "shaders/point_cloud.frag");
    GLuint vao; glGenVertexArrays(1, &vao);

    lastTime = glfwGetTime();

    while (!glfwWindowShouldClose(window)) {
        // 1. FPS 计算与显示
        double currentTime = glfwGetTime();
        nbFrames++;
        if (currentTime - lastTime >= 1.0) { // 每秒更新一次
            std::string title = "C++ Cloud Viewer | FPS: " + std::to_string(nbFrames) + 
                                " | Points: " + std::to_string(POINT_COUNT / 1000) + "K";
            glfwSetWindowTitle(window, title.c_str());
            nbFrames = 0;
            lastTime += 1.0;
        }

        // 2. 随机运动逻辑 (直接操作显存指针)
        // 模拟布朗运动，每一帧给点一个微小的偏移
        for(int i = 0; i < POINT_COUNT; i++) {
            mappedPtr[i].pos[0] += (rand() % 10 - 5) / 500.0f;
            mappedPtr[i].pos[1] += (rand() % 10 - 5) / 500.0f;
            mappedPtr[i].pos[2] += (rand() % 10 - 5) / 500.0f;
        }

        glClearColor(0.02f, 0.02f, 0.02f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // 相机矩阵计算
        glm::vec3 cameraPos;
        cameraPos.x = cameraDist * cos(glm::radians(pitch)) * cos(glm::radians(yaw));
        cameraPos.y = cameraDist * sin(glm::radians(pitch));
        cameraPos.z = cameraDist * cos(glm::radians(pitch)) * sin(glm::radians(yaw));

        glm::mat4 projection = glm::perspective(glm::radians(45.0f), 1280.0f/720.0f, 0.1f, 1000.0f);
        glm::mat4 view = glm::lookAt(cameraPos, glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f));

        pointShader.use();
        glUniformMatrix4fv(glGetUniformLocation(pointShader.ID, "uProjection"), 1, GL_FALSE, glm::value_ptr(projection));
        glUniformMatrix4fv(glGetUniformLocation(pointShader.ID, "uView"), 1, GL_FALSE, glm::value_ptr(view));
        
        glBindVertexArray(vao);
        glDrawArrays(GL_POINTS, 0, POINT_COUNT);

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glfwTerminate();
    return 0;
}

// #include <glad/glad.h>
// #include <GLFW/glfw3.h>
// #include <glm/glm.hpp>
// #include <glm/gtc/matrix_transform.hpp>
// #include <glm/gtc/type_ptr.hpp>
// #include <iostream>
// #include <vector>
// #include "renderer/Shader.hpp"

// // 1. 数据结构对齐
// struct Point {
//     alignas(16) float pos[4];
//     alignas(16) float color[4];
// };

// const int POINT_COUNT = 500000;

// // 简单的相机控制变量
// float cameraDist = 5.0f;
// float yaw = -90.0f, pitch = 0.0f;

// int main() {
//     // --- 基础初始化 ---
//     glfwInit();
//     glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
//     glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
//     GLFWwindow* window = glfwCreateWindow(1280, 720, "C++ Cloud: Persistent Mapping", NULL, NULL);
//     glfwMakeContextCurrent(window);
//     gladLoadGLLoader((GLADloadproc)glfwGetProcAddress);
//     glEnable(GL_PROGRAM_POINT_SIZE);
//     glEnable(GL_DEPTH_TEST);

//     // --- 2. 核心：Persistent Mapped SSBO ---
//     GLuint ssbo;
//     glGenBuffers(1, &ssbo);
//     glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssbo);
//     GLbitfield flags = GL_MAP_WRITE_BIT | GL_MAP_PERSISTENT_BIT | GL_MAP_COHERENT_BIT;
//     glBufferStorage(GL_SHADER_STORAGE_BUFFER, POINT_COUNT * sizeof(Point), nullptr, flags);
//     Point* mappedPtr = (Point*)glMapBufferRange(GL_SHADER_STORAGE_BUFFER, 0, POINT_COUNT * sizeof(Point), flags);

//     // 初始化百万个点（模拟 Lidar 静态背景）
//     for(int i = 0; i < POINT_COUNT; i++) {
//         mappedPtr[i].pos[0] = (rand() % 2000 - 1000) / 200.0f;
//         mappedPtr[i].pos[1] = (rand() % 2000 - 1000) / 200.0f;
//         mappedPtr[i].pos[2] = (rand() % 2000 - 1000) / 200.0f;
//         mappedPtr[i].pos[3] = 1.0f;
//     }
//     glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, ssbo);

//     Shader pointShader("shaders/point_cloud.vert", "shaders/point_cloud.frag");
//     GLuint vao; glGenVertexArrays(1, &vao);

//     // --- 3. 渲染循环 ---
//     while (!glfwWindowShouldClose(window)) {
//         glClearColor(0.02f, 0.02f, 0.02f, 1.0f);
//         glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

//         // A. 计算相机矩阵 (类似 OrbitControls)
//         float time = glfwGetTime();
//         glm::mat4 projection = glm::perspective(glm::radians(45.0f), 1280.0f/720.0f, 0.1f, 1000.0f);
        
//         // 自动旋转相机演示效果
//         float camX = sin(time * 0.5f) * cameraDist;
//         float camZ = cos(time * 0.5f) * cameraDist;
//         glm::mat4 view = glm::lookAt(glm::vec3(camX, 2.0f, camZ), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f));

//         // B. 数据实时更新演示 (模拟动态障碍物)
//         // 就像 SharedArrayBuffer 一样直接改内存！
//         for(int i=0; i<5000; i++) {
//             mappedPtr[i].pos[1] += sin(time + i) * 0.005f; 
//         }

//         // C. 提交渲染
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