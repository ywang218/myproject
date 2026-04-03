#pragma once
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <cmath> // 必须包含 cmath 才能使用 cos/sin

class Camera {
public:
    glm::vec3 Position;
    glm::vec3 Front;
    glm::vec3 Up;

    // 直接在头文件实现构造函数
    Camera(glm::vec3 position = glm::vec3(0.0f, -60.0f, 20.0f)) 
        : Position(position), Front(glm::vec3(1.0f, 0.0f, 0.0f)), Up(glm::vec3(0.0f, 0.0f, 1.0f)) {}

    // GetViewMatrix 实现
    glm::mat4 GetViewMatrix() { 
        return glm::lookAt(Position, Position + Front, Up); 
    }
    
    // GetProjectionMatrix 实现
    glm::mat4 GetProjectionMatrix(float w, float h) {
        if (h == 0) return glm::mat4(1.0f); // 防止除零异常
        return glm::perspective(glm::radians(60.0f), w / h, 0.1f, 1000.0f);
    }
    
    // updateFollow 实现 (平滑跟随逻辑)
    void updateFollow(glm::vec3 egoPos, float yaw, float dist, float height, float lerpFactor) {
        // 1. 计算目标追随位置
        glm::vec3 forward(cos(yaw), sin(yaw), 0.0f);
        glm::vec3 targetPos = egoPos - forward * dist + glm::vec3(0, 0, height);
        
        // 2. 线性插值位置
        Position = glm::mix(Position, targetPos, lerpFactor);
        
        // 3. 始终看向主车前方 10 米
        glm::vec3 lookAtTarget = egoPos + forward * 6.0f;
        Front = glm::normalize(lookAtTarget - Position);
    }
};