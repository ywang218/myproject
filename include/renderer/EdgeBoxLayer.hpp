#pragma once
#include <glad/glad.h>
#include <glm/glm.hpp>
#include "renderer/Shader.hpp"
#include <memory>

class EdgeBoxLayer {
public:
    EdgeBoxLayer() {
        // 定义立方体的 8 个顶点
        float vertices[] = {
            -0.5f,-0.5f,-0.5f,  0.5f,-0.5f,-0.5f,  0.5f, 0.5f,-0.5f, -0.5f, 0.5f,-0.5f,
            -0.5f,-0.5f, 0.5f,  0.5f,-0.5f, 0.5f,  0.5f, 0.5f, 0.5f, -0.5f, 0.5f, 0.5f
        };
        // 12 条边的索引
        unsigned int indices[] = {
            0,1, 1,2, 2,3, 3,0, // 底面
            4,5, 5,6, 6,7, 7,4, // 顶面
            0,4, 1,5, 2,6, 3,7  // 侧边
        };

        glGenVertexArrays(1, &vao);
        glGenBuffers(1, &vbo);
        glGenBuffers(1, &ebo);

        glBindVertexArray(vao);
        glBindBuffer(GL_ARRAY_BUFFER, vbo);
        glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
        
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(0);
        
        shader = std::make_unique<Shader>("shaders/edgebox.vert", "shaders/edgebox.frag");
    }

    void render(glm::vec3 pos, float yaw, glm::vec3 size, const glm::mat4& view, const glm::mat4& proj) {
        glLineWidth(2.0f); // 让线粗一点，更容易看见
        shader->use();
        glm::mat4 model = glm::mat4(1.0f);
        model = glm::translate(model, pos);
        // 注意：如果是沿 Z 轴旋转（朝向），通常用 glm::vec3(0, 0, 1)
        model = glm::rotate(model, yaw, glm::vec3(0, 0, 1)); 
        model = glm::scale(model, size);
        // ----------------------------

        shader->setMat4("model", model);
        shader->setMat4("view", view);
        shader->setMat4("projection", proj);
        
        // 确保 Alpha 是 1.0
        shader->setVec4("color", glm::vec4(1.0f, 0.0f, 0.0f, 1.0f)); 

        glBindVertexArray(vao);
        glDrawElements(GL_LINES, 24, GL_UNSIGNED_INT, 0); // 使用 GL_LINES
    }
private:
    unsigned int vao, vbo, ebo;
    std::unique_ptr<Shader> shader;
};