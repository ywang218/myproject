#pragma once
#include <glad/glad.h>
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <vector>
#include <memory>
#include "renderer/Shader.hpp"
#include "data/MockFrameGenerator.hpp" 

class ObstacleEdgeLayer {
public:
    // 构造函数：初始化 OpenGL 资源
    ObstacleEdgeLayer(int maxCount = 1000) : maxSize(maxCount), currentCount(0) {
        
        // --- 1. 定义几何体 (所有障碍物共用的 8 个顶点) ---
      float vertices[] = {
        // 前平面 (Z = 0.5)
        -0.5f, -0.5f,  0.5f, // 0: 左下前
        0.5f, -0.5f,  0.5f, // 1: 右下前
        0.5f,  0.5f,  0.5f, // 2: 右上前
        -0.5f,  0.5f,  0.5f, // 3: 左上前
        // 后平面 (Z = -0.5)
        -0.5f, -0.5f, -0.5f, // 4: 左下后
        0.5f, -0.5f, -0.5f, // 5: 右下后
        0.5f,  0.5f, -0.5f, // 6: 右上后
        -0.5f,  0.5f, -0.5f  // 7: 左上后
    };
        // unsigned int indices[] = {
        //     0,1,2, 2,3,0, 1,5,6, 6,2,1, 7,6,5, 5,4,7,
        //     4,0,3, 3,7,4, 4,5,1, 1,0,4, 3,2,6, 6,7,3
        // };
         unsigned int indices[] = {
            0,1, 1,2, 2,3, 3,0, // 底面
            4,5, 5,6, 6,7, 7,4, // 顶面
            0,4, 1,5, 2,6, 3,7  // 侧边
        }; 


        // --- 2. 创建并绑定 GPU 缓冲区 ---
        glGenVertexArrays(1, &vao);
        glGenBuffers(1, &vbo);
        glGenBuffers(1, &ebo);
        glGenBuffers(1, &instanceVBO); // <--- 这里就是给 instanceVBO 分配 ID

        glBindVertexArray(vao);

        // 顶点数据 (VBO)
        glBindBuffer(GL_ARRAY_BUFFER, vbo);
        glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(0);

        // 索引数据 (EBO)
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

        // --- 3. 设置实例化缓冲区 (关键点) ---
        glBindBuffer(GL_ARRAY_BUFFER, instanceVBO);
        size_t totalStride = sizeof(glm::mat4) + sizeof(glm::vec4);
        glBufferData(GL_ARRAY_BUFFER, maxSize * totalStride, nullptr, GL_DYNAMIC_DRAW);

        // 矩阵占 4 个 location (1, 2, 3, 4)
        for (int i = 0; i < 4; i++) {
            glEnableVertexAttribArray(1 + i);
            glVertexAttribPointer(1 + i, 4, GL_FLOAT, GL_FALSE, totalStride, (void*)(i * sizeof(glm::vec4)));
            glVertexAttribDivisor(1 + i, 1); 
        }

        // 颜色占 1 个 location (5)
        glEnableVertexAttribArray(5);
        glVertexAttribPointer(5, 4, GL_FLOAT, GL_FALSE, totalStride, (void*)sizeof(glm::mat4));
        glVertexAttribDivisor(5, 1); 

        dataPayload.reserve(maxSize * 20);
        // 加载 Shader (你需要新建这两个文件)
        shader = std::make_unique<Shader>("shaders/obstacle.vert", "shaders/obstacle.frag");
    }

    // 析构函数：释放内存，防止内存泄漏
    ~ObstacleEdgeLayer() {
        glDeleteVertexArrays(1, &vao);
        glDeleteBuffers(1, &vbo);
        glDeleteBuffers(1, &ebo);
        glDeleteBuffers(1, &instanceVBO);
    }

    // 上传由 Renderer 预计算好的实例数据 (与填充层共用同一份 mat4 + vec4)，本层不再重复算矩阵。
    void uploadInstances(const std::vector<float>& payload, int count) {
        currentCount = std::min(count, maxSize);
        if (currentCount == 0) return;

        glBindBuffer(GL_ARRAY_BUFFER, instanceVBO);
        glBufferSubData(GL_ARRAY_BUFFER, 0,
                        (GLsizeiptr)(currentCount * 20 * sizeof(float)), payload.data());
    }

    // 渲染函数
    void render(const glm::mat4& view, const glm::mat4& proj) {
        if (currentCount == 0) return;
        shader->use();
        shader->setMat4("view", view);
        shader->setMat4("projection", proj);
        shader->setFloat("uAlpha", 1.0f); // 边框不透明
        glBindVertexArray(vao);
        glLineWidth(3.0f);

        // --- 核心修复：开启多边形偏移 ---
        glEnable(GL_POLYGON_OFFSET_LINE);
        // 负值代表将线条向“近处”拉，正值向远处推
        glPolygonOffset(-1.0f, -1.0f);
        glDrawElementsInstanced(GL_LINES, 24, GL_UNSIGNED_INT, 0, currentCount);
        glDisable(GL_POLYGON_OFFSET_LINE);
    }

private:
    // --- 这里是你在 C++ 中必须声明的所有成员变量 ---
    unsigned int vao;
    unsigned int vbo;
    unsigned int ebo;
    unsigned int instanceVBO; // <--- 在这里声明，函数里才能用 & 取地址
    
    int maxSize;
    int currentCount;
    std::unique_ptr<Shader> shader;

    std::vector<float> dataPayload;
};