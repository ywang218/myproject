#pragma once
#include <glad/glad.h>
#include <glm/glm.hpp>
#include <vector>
#include <memory>
#include "renderer/Shader.hpp"

struct BeltVertex {
    glm::vec3 pos;
    glm::vec3 prev;
    glm::vec3 next;
    float side;
    float lineDist;
    glm::vec3 color;
    float dash;
    float width;
};

class BeltBatch {
public:
    BeltBatch(int maxVerts = 100000) {
        shader = std::make_unique<Shader>("shaders/belt.vert", "shaders/belt.frag");
        
        glGenVertexArrays(1, &vao);
        glGenBuffers(1, &vbo);
        glGenBuffers(1, &ebo);

        glBindVertexArray(vao);
        glBindBuffer(GL_ARRAY_BUFFER, vbo);
        // 预分配内存
        glBufferData(GL_ARRAY_BUFFER, maxVerts * sizeof(BeltVertex), nullptr, GL_DYNAMIC_DRAW);

        // 设置 Layout (对应 Shader 中的 location)
        auto setupAttr = [](int index, int size, size_t offset) {
            glEnableVertexAttribArray(index);
            glVertexAttribPointer(index, size, GL_FLOAT, GL_FALSE, sizeof(BeltVertex), (void*)offset);
        };

        setupAttr(0, 3, offsetof(BeltVertex, pos));
        setupAttr(1, 3, offsetof(BeltVertex, prev));
        setupAttr(2, 3, offsetof(BeltVertex, next));
        setupAttr(3, 1, offsetof(BeltVertex, side));
        setupAttr(4, 1, offsetof(BeltVertex, lineDist));
        setupAttr(5, 3, offsetof(BeltVertex, color));
        setupAttr(6, 1, offsetof(BeltVertex, dash));
        setupAttr(7, 1, offsetof(BeltVertex, width));

        // 预生成索引缓冲区 (每 2 个点生成 2 个三角形)
        std::vector<unsigned int> indices;
        for (unsigned int i = 0; i < maxVerts - 2; i += 2) {
            indices.push_back(i); indices.push_back(i+1); indices.push_back(i+2);
            indices.push_back(i+2); indices.push_back(i+1); indices.push_back(i+3);
        }
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), indices.data(), GL_STATIC_DRAW);
    }

    void begin() {
        vertices.clear();
    }

    void writeBelt(const std::vector<glm::vec3>& points, float offsetY, float width, bool dashed, glm::vec3 color) {
        if (points.size() < 2) return;
        float dist = 0.0f;

        for (int i = 0; i < points.size(); ++i) {
            glm::vec3 curr = points[i];
            curr.y += offsetY; // 对应 JS 的 offsetY
            glm::vec3 prev = (i > 0) ? points[i-1] : curr;
            glm::vec3 next = (i < points.size() - 1) ? points[i+1] : curr;

            if (i > 0) dist += glm::distance(points[i], points[i-1]);

            // 每个轨迹点生成 2 个顶点 (左和右)
            for (int s = 0; s < 2; ++s) {
                BeltVertex v;
                v.pos = curr; v.prev = prev; v.next = next;
                v.side = (s == 0) ? -1.0f : 1.0f;
                v.lineDist = dist;
                v.color = color;
                v.dash = dashed ? 1.0f : 0.0f;
                v.width = width;
                vertices.push_back(v);
            }
        }

        // 添加 4 个退化顶点来断开条带
        if (!vertices.empty()) {
            BeltVertex last = vertices.back();
            last.side = 0; // 让 Shader 不挤出，面积归零
            for (int i = 0; i < 4; ++i) vertices.push_back(last);
        }
    }

    void render(const glm::mat4& view, const glm::mat4& proj) {
        if (vertices.empty()) return;

        shader->use();
        shader->setMat4("view", view);
        shader->setMat4("projection", proj);

        glBindBuffer(GL_ARRAY_BUFFER, vbo);
        glBufferSubData(GL_ARRAY_BUFFER, 0, vertices.size() * sizeof(BeltVertex), vertices.data());

        glBindVertexArray(vao);
        int indexCount = (vertices.size() > 2) ? (vertices.size() - 2) * 3 : 0;
        glDrawElements(GL_TRIANGLES, indexCount, GL_UNSIGNED_INT, 0);
    }

private:
    unsigned int vao, vbo, ebo;
    std::vector<BeltVertex> vertices;
    std::unique_ptr<Shader> shader;
};