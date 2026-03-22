#pragma once
#include <glad/glad.h>
#include <glm/glm.hpp>
#include "renderer/Shader.hpp"
#include <memory>

class BoxLayer {
public:
    BoxLayer() {
        float vertices[] = {
            -0.5f, -0.5f, -0.5f,  0.5f, -0.5f, -0.5f,  0.5f,  0.5f, -0.5f,
             0.5f,  0.5f, -0.5f, -0.5f,  0.5f, -0.5f, -0.5f, -0.5f, -0.5f,
            // ... 省略其他 5 个面，通常定义 36 个顶点 ...
        };
        glGenVertexArrays(1, &vao);
        glGenBuffers(1, &vbo);
        glBindVertexArray(vao);
        glBindBuffer(GL_ARRAY_BUFFER, vbo);
        glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(0);
        
        shader = std::make_unique<Shader>("shaders/box.vert", "shaders/box.frag");
    }

    void render(glm::vec3 pos, float yaw, glm::vec3 size, const glm::mat4& view, const glm::mat4& proj) {
        shader->use();
        glm::mat4 model = glm::mat4(1.0f);
        model = glm::translate(model, pos);
        model = glm::rotate(model, yaw, glm::vec3(0, 0, 1));
        model = glm::scale(model, size);
        
        shader->setMat4("model", model);
        shader->setMat4("view", view);
        shader->setMat4("projection", proj);
        shader->setVec4("color", glm::vec4(0.0f, 1.0f, 0.5f, 1.0f)); // 对应 JS 主车颜色

        glBindVertexArray(vao);
        glDrawArrays(GL_TRIANGLES, 0, 36);
    }

private:
    unsigned int vao, vbo;
    std::unique_ptr<Shader> shader;
};