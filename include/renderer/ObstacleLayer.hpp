// #pragma once
// #include <glad/glad.h>
// #include <glm/glm.hpp>
// #include <glm/gtc/type_ptr.hpp>
// #include <vector>
// #include <memory>
// #include "renderer/Shader.hpp"
// #include "data/MockFrameGenerator.hpp" 

// // https://chatgpt.com/c/69cdca63-d2dc-83a5-9366-481e03fa16e2

// class ObstacleLayer {
// public:
//     // 构造函数：初始化 OpenGL 资源
//     ObstacleLayer(int maxCount = 1000) : maxSize(maxCount), currentCount(0) {
        
//         // --- 1. 定义几何体 (所有障碍物共用的 8 个顶点) ---
//         float vertices[] = {
//             -0.5f, -0.5f,  0.5f,  0.5f, -0.5f,  0.5f,  0.5f,  0.5f,  0.5f, -0.5f,  0.5f,  0.5f,
//             -0.5f, -0.5f, -0.5f,  0.5f, -0.5f, -0.5f,  0.5f,  0.5f, -0.5f, -0.5f,  0.5f, -0.5f
//         };
//         unsigned int indices[] = {
//             0,1,2, 2,3,0, 1,5,6, 6,2,1, 7,6,5, 5,4,7,
//             4,0,3, 3,7,4, 4,5,1, 1,0,4, 3,2,6, 6,7,3
//         };

//         // --- 2. 创建并绑定 GPU 缓冲区 ---
//         glGenVertexArrays(1, &vao);
//         glGenBuffers(1, &vbo);
//         glGenBuffers(1, &ebo);
//         glGenBuffers(1, &instanceVBO); // <--- 这里就是给 instanceVBO 分配 ID

//         glBindVertexArray(vao);

//         // 顶点数据 (VBO)
//         glBindBuffer(GL_ARRAY_BUFFER, vbo);
//         glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
//         glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
//         glEnableVertexAttribArray(0);

//         // 索引数据 (EBO)
//         glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
//         glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

//         // --- 3. 设置实例化缓冲区 (关键点) ---
//         glBindBuffer(GL_ARRAY_BUFFER, instanceVBO);
//         size_t totalStride = sizeof(glm::mat4) + sizeof(glm::vec4);
//         glBufferData(GL_ARRAY_BUFFER, maxSize * totalStride, nullptr, GL_DYNAMIC_DRAW);

//         // 矩阵占 4 个 location (1, 2, 3, 4)
//         for (int i = 0; i < 4; i++) {
//             glEnableVertexAttribArray(1 + i);
//             glVertexAttribPointer(1 + i, 4, GL_FLOAT, GL_FALSE, totalStride, (void*)(i * sizeof(glm::vec4)));
//             glVertexAttribDivisor(1 + i, 1); 
//         }

//         // 颜色占 1 个 location (5)
//         glEnableVertexAttribArray(5);
//         glVertexAttribPointer(5, 4, GL_FLOAT, GL_FALSE, totalStride, (void*)sizeof(glm::mat4));
//         glVertexAttribDivisor(5, 1); 

//         dataPayload.reserve(maxSize * 20);
//         // 加载 Shader (你需要新建这两个文件)
//         shader = std::make_unique<Shader>("shaders/obstacle.vert", "shaders/obstacle.frag");
//     }

//     // 析构函数：释放内存，防止内存泄漏
//     ~ObstacleLayer() {
//         glDeleteVertexArrays(1, &vao);
//         glDeleteBuffers(1, &vbo);
//         glDeleteBuffers(1, &ebo);
//         glDeleteBuffers(1, &instanceVBO);
//     }

//     // 更新函数
//     void updateData(const std::vector<Polygon>& obstacles) {
//     //    std::cout << "--- Current Obstacles (Count: " << obstacles.size() << ") ---" << std::endl;

//     //     for (const auto& obs : obstacles) {
//     //         std::cout << "ID: " << obs.id 
//     //                 << " | Pos: (" << obs.center.x << ", " << obs.center.y << ", " << obs.center.z << ")"
//     //                 << " | Heading: " << obs.heading 
//     //                 << std::endl;
//     //     }

//     //     std::cout << "------------------------------------------" << std::endl;
//         lastObstacles = obstacles;
//         currentCount = std::min((int)obstacles.size(), maxSize);

//         if (currentCount == 0) return;

//         // std::vector<float> dataPayload;
//         // dataPayload.reserve(currentCount * 20);

//         dataPayload.clear();

//         for (int i = 0; i < currentCount; i++) {
//             const auto& obs = obstacles[i];
//             glm::mat4 model = glm::mat4(1.0f);
//             model = glm::translate(model, obs.center);
//             model = glm::rotate(model, obs.heading, glm::vec3(0, 0, 1));
//             model = glm::scale(model, obs.size);

//             const float* mPtr = glm::value_ptr(model);
//             dataPayload.insert(dataPayload.end(), mPtr, mPtr + 16);

//             // 写入颜色 (4 floats)
//             dataPayload.push_back(obs.style.color.r);
//             dataPayload.push_back(obs.style.color.g);
//             dataPayload.push_back(obs.style.color.b);
//             dataPayload.push_back(0.6f);
//             // for(int j=0; j<16; j++) dataPayload.push_back(mPtr[j]);

//             // dataPayload.push_back(obs.style.color.r);
//             // dataPayload.push_back(obs.style.color.g);
//             // dataPayload.push_back(obs.style.color.b);
//             // dataPayload.push_back(0.6f); // Alpha
//         }

//         glBindBuffer(GL_ARRAY_BUFFER, instanceVBO);
        
//         // 关键：传 NULL 告诉驱动程序“旧数据不要了，直接给我在显存开新空间”
//         // 这样 CPU 就不需要等待上一帧渲染结束，彻底消除卡顿
//         size_t totalBufferSize = maxSize * (sizeof(glm::mat4) + sizeof(glm::vec4));
//         glBufferData(GL_ARRAY_BUFFER, totalBufferSize, nullptr, GL_DYNAMIC_DRAW); 

//         // 上传新数据
//         glBufferSubData(GL_ARRAY_BUFFER, 0, dataPayload.size() * sizeof(float), dataPayload.data());
            
//         // glBufferSubData(GL_ARRAY_BUFFER, 0, dataPayload.size() * sizeof(float), dataPayload.data());
//     }

//     // --- 新增：射线拾取算法 ---
//     int pickObstacle(glm::vec3 rayOrigin, glm::vec3 rayDir) {
//         float closestT = 1e10f;
//         int hitIdx = -1;

//         for (int i = 0; i < currentCount; i++) {
//             const auto& obs = lastObstacles[i];
//             // 使用简化的球体包围盒进行快速检测
//             glm::vec3 sphereCenter = obs.center;
//             float sphereRadius = glm::max(glm::max(obs.size.x, obs.size.y), obs.size.z) * 0.5f;

//             glm::vec3 oc = rayOrigin - sphereCenter;
//             float b = glm::dot(oc, rayDir);
//             float c = glm::dot(oc, oc) - sphereRadius * sphereRadius;
//             float h = b * b - c;
            
//             if (h >= 0.0f) {
//                 float t = -b - sqrt(h);
//                 if (t > 0 && t < closestT) {
//                     closestT = t;
//                     hitIdx = i;
//                 }
//             }
//         }
//         return hitIdx;
//     }

//     const Polygon* getObstacle(int index) const {
//         if (index < 0 || index >= (int)lastObstacles.size()) return nullptr;
//         return &lastObstacles[index];
//     }

//     // 渲染函数
//     void render(const glm::mat4& view, const glm::mat4& proj) {
//         if (currentCount == 0) return;
//         shader->use();
//         shader->setMat4("view", view);
//         shader->setMat4("projection", proj);
//         glBindVertexArray(vao);
//         glDrawElementsInstanced(GL_TRIANGLES, 36, GL_UNSIGNED_INT, 0, currentCount);
//     }

// private:
//     // --- 这里是你在 C++ 中必须声明的所有成员变量 ---
//     unsigned int vao;
//     unsigned int vbo;
//     unsigned int ebo;
//     unsigned int instanceVBO; // <--- 在这里声明，函数里才能用 & 取地址
    
//     int maxSize;
//     int currentCount;
//     std::unique_ptr<Shader> shader;

//     std::vector<float> dataPayload;
//     std::vector<Polygon> lastObstacles; // CPU 备份副本
// };

#pragma once
#include <glad/glad.h>
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <vector>
#include <memory>
#include <algorithm>
#include "renderer/Shader.hpp"
#include "data/MockFrameGenerator.hpp" 

class ObstacleLayer {
public:
    ObstacleLayer(int maxCount = 1000) : maxSize(maxCount), currentCount(0) {
        float vertices[] = {
            -0.5f, -0.5f,  0.5f,  0.5f, -0.5f,  0.5f,  0.5f,  0.5f,  0.5f, -0.5f,  0.5f,  0.5f,
            -0.5f, -0.5f, -0.5f,  0.5f, -0.5f, -0.5f,  0.5f,  0.5f, -0.5f, -0.5f,  0.5f, -0.5f
        };
        unsigned int indices[] = {
            0,1,2, 2,3,0, 1,5,6, 6,2,1, 7,6,5, 5,4,7,
            4,0,3, 3,7,4, 4,5,1, 1,0,4, 3,2,6, 6,7,3
        };

        glGenVertexArrays(1, &vao);
        glGenBuffers(1, &vbo);
        glGenBuffers(1, &ebo);
        glGenBuffers(1, &instanceVBO);

        glBindVertexArray(vao);
        glBindBuffer(GL_ARRAY_BUFFER, vbo);
        glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(0);

        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

        glBindBuffer(GL_ARRAY_BUFFER, instanceVBO);
        size_t totalStride = sizeof(glm::mat4) + sizeof(glm::vec4);
        glBufferData(GL_ARRAY_BUFFER, maxSize * totalStride, nullptr, GL_DYNAMIC_DRAW);

        for (int i = 0; i < 4; i++) {
            glEnableVertexAttribArray(1 + i);
            glVertexAttribPointer(1 + i, 4, GL_FLOAT, GL_FALSE, (GLsizei)totalStride, (void*)(i * sizeof(glm::vec4)));
            glVertexAttribDivisor(1 + i, 1); 
        }

        glEnableVertexAttribArray(5);
        glVertexAttribPointer(5, 4, GL_FLOAT, GL_FALSE, (GLsizei)totalStride, (void*)sizeof(glm::mat4));
        glVertexAttribDivisor(5, 1); 

        shader = std::make_unique<Shader>("shaders/obstacle.vert", "shaders/obstacle.frag");
    }

    ~ObstacleLayer() {
        glDeleteVertexArrays(1, &vao);
        glDeleteBuffers(1, &vbo);
        glDeleteBuffers(1, &ebo);
        glDeleteBuffers(1, &instanceVBO);
    }

    void updateData(const std::vector<Polygon>& obstacles) {
        lastObstacles = obstacles; 
        currentCount = std::min((int)obstacles.size(), maxSize);
        if (currentCount == 0) return;

        dataPayload.clear();
        for (int i = 0; i < currentCount; i++) {
            const auto& obs = obstacles[i];
            glm::mat4 model = glm::mat4(1.0f);
            model = glm::translate(model, obs.center);
            model = glm::rotate(model, obs.heading, glm::vec3(0, 0, 1));
            model = glm::scale(model, obs.size);

            const float* mPtr = glm::value_ptr(model);
            dataPayload.insert(dataPayload.end(), mPtr, mPtr + 16);
            dataPayload.push_back(obs.style.color.r);
            dataPayload.push_back(obs.style.color.g);
            dataPayload.push_back(obs.style.color.b);
            dataPayload.push_back(0.6f);
        }

        glBindBuffer(GL_ARRAY_BUFFER, instanceVBO);
        glBufferSubData(GL_ARRAY_BUFFER, 0, dataPayload.size() * sizeof(float), dataPayload.data());
    }

    int pickObstacle(glm::vec3 rayOrigin, glm::vec3 rayDir) {
        float closestT = 1e10f;
        int hitIdx = -1;
        for (int i = 0; i < currentCount; i++) {
            const auto& obs = lastObstacles[i];
            glm::vec3 oc = rayOrigin - obs.center;
            // 碰撞检测半径：取物体的最大包围长度
            float sphereRadius = glm::max(glm::max(obs.size.x, obs.size.y), obs.size.z) * 0.5f;
            float b = glm::dot(oc, rayDir);
            float c = glm::dot(oc, oc) - sphereRadius * sphereRadius;
            float h = b * b - c;
            if (h >= 0.0f) {
                float t = -b - sqrt(h);
                if (t > 0 && t < closestT) { closestT = t; hitIdx = i; }
            }
        }
        return hitIdx;
    }

    const Polygon* getObstacle(int index) const {
        if (index < 0 || index >= (int)lastObstacles.size()) return nullptr;
        return &lastObstacles[index];
    }

    void render(const glm::mat4& view, const glm::mat4& proj) {
        if (currentCount == 0) return;
        shader->use();
        shader->setMat4("view", view);
        shader->setMat4("projection", proj);
        glBindVertexArray(vao);
        glDrawElementsInstanced(GL_TRIANGLES, 36, GL_UNSIGNED_INT, 0, currentCount);
    }

private:
    unsigned int vao, vbo, ebo, instanceVBO;
    int maxSize, currentCount;
    std::unique_ptr<Shader> shader;
    std::vector<float> dataPayload;
    std::vector<Polygon> lastObstacles;
};