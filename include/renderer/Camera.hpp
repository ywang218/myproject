#pragma once
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

class Camera {
public:
    // 基础属性 (对齐 Renderer.js 的初始值)
    glm::vec3 Position;
    glm::vec3 Target = glm::vec3(0.0f);
    glm::vec3 Up = glm::vec3(0.0f, 1.0f, 0.0f);

    float Fov = 60.0f;
    float Near = 0.1f;
    float Far = 1000.0f;
    bool isOrtho = false;
    float orthoSize = 50.0f;

    Camera(glm::vec3 pos = glm::vec3(0.0f, 15.0f, 25.0f)) : Position(pos) {}

    // 对应 Three.js 的 updateProjectionMatrix()
    glm::mat4 GetProjectionMatrix(float screenWidth, float screenHeight) {
        float aspect = screenWidth / screenHeight;
        if (!isOrtho) {
            return glm::perspective(glm::radians(Fov), aspect, Near, Far);
        } else {
            // 对齐你的 _syncOrthoWithPerspective 逻辑
            float h = orthoSize;
            float w = h * aspect;
            return glm::ortho(-w/2.0f, w/2.0f, -h/2.0f, h/2.0f, Near, Far);
        }
    }

    // 对应 OrbitControls 的基本效果
    glm::mat4 GetViewMatrix() {
        return glm::lookAt(Position, Target, Up);
    }
};