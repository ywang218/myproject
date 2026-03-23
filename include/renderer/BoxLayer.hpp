// #pragma once
// #include <glad/glad.h>
// #include <glm/glm.hpp>
// #include "renderer/Shader.hpp"
// #include <memory>

// class BoxLayer {
// public:
//     BoxLayer() {
//         float vertices[] = {
//             -0.5f, -0.5f, -0.5f,  0.5f, -0.5f, -0.5f,  0.5f,  0.5f, -0.5f,
//              0.5f,  0.5f, -0.5f, -0.5f,  0.5f, -0.5f, -0.5f, -0.5f, -0.5f,
//             // ... 省略其他 5 个面，通常定义 36 个顶点 ...
//         };
//         glGenVertexArrays(1, &vao);
//         glGenBuffers(1, &vbo);
//         glBindVertexArray(vao);
//         glBindBuffer(GL_ARRAY_BUFFER, vbo);
//         glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
//         glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
//         glEnableVertexAttribArray(0);
        
//         shader = std::make_unique<Shader>("shaders/box.vert", "shaders/box.frag");
//     }

//     void render(glm::vec3 pos, float yaw, glm::vec3 size, const glm::mat4& view, const glm::mat4& proj) {
//         shader->use();
//         glm::mat4 model = glm::mat4(1.0f);
//         model = glm::translate(model, pos);
//         model = glm::rotate(model, yaw, glm::vec3(0, 0, 1));
//         model = glm::scale(model, size);
        
//         shader->setMat4("model", model);
//         shader->setMat4("view", view);
//         shader->setMat4("projection", proj);
//         shader->setVec4("color", glm::vec4(0.0f, 1.0f, 0.5f, 1.0f)); // 对应 JS 主车颜色

//         glBindVertexArray(vao);
//         glDrawArrays(GL_TRIANGLES, 0, 36);
//     }

// private:
//     unsigned int vao, vbo;
//     std::unique_ptr<Shader> shader;
// };

#pragma once
#include <glad/glad.h>
#include <glm/glm.hpp>
#include "renderer/Shader.hpp"
#include <memory>

class BoxLayer {
public:
    BoxLayer() {
        // 1. 定义立方体的 8 个唯一顶点 (模型坐标系：-0.5 到 0.5)
        float vertices[] = {
            // 前平面顶点 (z = 0.5)
            -0.5f, -0.5f,  0.5f,  // 0: 左下前
             0.5f, -0.5f,  0.5f,  // 1: 右下前
             0.5f,  0.5f,  0.5f,  // 2: 右上前
            -0.5f,  0.5f,  0.5f,  // 3: 左上前
            // 后平面顶点 (z = -0.5)
            -0.5f, -0.5f, -0.5f,  // 4: 左下后
             0.5f, -0.5f, -0.5f,  // 5: 右下后
             0.5f,  0.5f, -0.5f,  // 6: 右上后
            -0.5f,  0.5f, -0.5f   // 7: 左上后
        };

        // 2. 定义 36 个索引 (12 个三角形，组成 6 个面)
        unsigned int indices[] = {
            0, 1, 2,  2, 3, 0, // 前面
            1, 5, 6,  6, 2, 1, // 右面
            7, 6, 5,  5, 4, 7, // 后面
            4, 0, 3,  3, 7, 4, // 左面
            4, 5, 1,  1, 0, 4, // 底面
            3, 2, 6,  6, 7, 3  // 顶面
        };

        glGenVertexArrays(1, &vao);
        glGenBuffers(1, &vbo);
        glGenBuffers(1, &ebo); // 生成 EBO

        glBindVertexArray(vao);

        // 绑定并上传顶点数据 (VBO)
        glBindBuffer(GL_ARRAY_BUFFER, vbo);
        glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

        // 绑定并上传索引数据 (EBO)
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

        // 设置顶点属性指针 (位置: location = 0)
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(0);

        shader = std::make_unique<Shader>("shaders/box.vert", "shaders/box.frag");
    }

    ~BoxLayer() {
        glDeleteVertexArrays(1, &vao);
        glDeleteBuffers(1, &vbo);
        glDeleteBuffers(1, &ebo);
    }

    void render(glm::vec3 pos, float yaw, glm::vec3 size, const glm::mat4& view, const glm::mat4& proj) {
        shader->use();
        
        glm::mat4 model = glm::mat4(1.0f);
        model = glm::translate(model, pos);
        // 如果是自动驾驶场景，通常沿 Z 轴（高度轴）旋转
        model = glm::rotate(model, yaw, glm::vec3(0, 0, 1));
        model = glm::scale(model, size);
        
        shader->setMat4("model", model);
        shader->setMat4("view", view);
        shader->setMat4("projection", proj);
        // 设置一个略带透明感的青绿色（主车色）
        shader->setVec4("color", glm::vec4(0.0f, 1.0f, 0.5f, 0.8f)); 

        glBindVertexArray(vao);
        // 【关键变化】：使用 glDrawElements 而不是 glDrawArrays
        glDrawElements(GL_TRIANGLES, 36, GL_UNSIGNED_INT, 0);
    }

private:
    unsigned int vao, vbo, ebo;
    std::unique_ptr<Shader> shader;
};