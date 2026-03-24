
// #pragma once
// #include <glad/glad.h>
// #include <glm/glm.hpp>
// #include <memory>
// #include "renderer/Shader.hpp"
// #include "Point.hpp"

// // 结构体定义，确保与 Backend 一致
// // struct Point {
// //     glm::vec4 pos;   // x, y, z, intensity
// //     glm::vec4 color; // r, g, b, a
// // };

// class PointCloudLayer {
// public:
//     PointCloudLayer(int count);
//     ~PointCloudLayer();

//     // 关键：暴露这个指针给 Backend，让 Backend 能直接写显存
//     Point* getMappedPointer() { return sharedMappedPtr; }

//     // 设置抽稀倍率：1 表示全画，10 表示每 10 个点画 1 个
//     void setLOD(int stride) { 
//         currentStride = (stride > 0) ? stride : 1; 
//     } 

//     // 渲染逻辑
//     void render(const glm::mat4& view, const glm::mat4& projection, const glm::vec3& egoPos);

// private:
//     int pointCount;
//     int currentStride = 1;
//     GLuint ssbo, vao;
//     std::unique_ptr<Shader> renderShader;

//     // 指向 Persistent Mapping 显存地址的指针
//     Point* sharedMappedPtr = nullptr;

//     GLuint inputSSBO;   // 原有的 Persistent Mapping 缓冲区
//     GLuint outputSSBO;  // 新增：过滤后的数据
//     GLuint atomicBuffer;// 新增：存储 count 的缓冲区
//     GLuint filterShader;// 新增：Compute Shader 程序
    
//     int maxPoints;
// };


#pragma once
#include <glad/glad.h>
#include <glm/glm.hpp>
#include <memory>
#include "renderer/Shader.hpp"
#include "Point.hpp"

class PointCloudLayer {
public:
    PointCloudLayer(int count);
    ~PointCloudLayer();

    // 暴露给 Backend 的原始输入指针 (10M点全量写入)
    Point* getMappedPointer() { return sharedMappedPtr; }

    // 渲染逻辑：现在需要传入 egoPos 供 Compute Shader 计算距离
    void render(const glm::mat4& view, const glm::mat4& projection, const glm::vec3& egoPos);

private:
    int pointCount; // 总点数 (如 10,000,000)
    
    // --- 资源句柄 ---
    GLuint vao;
    GLuint inputSSBO;    // 原有的 Persistent Mapping 缓冲区
    GLuint outputSSBO;   // 过滤后的紧凑缓冲区
    GLuint atomicBuffer; // 存储有效点数的计数器
    GLint uEgoPosLoc, uTotalPointsLoc;

    std::unique_ptr<Shader> renderShader; // 顶点/片元着色器
    std::unique_ptr<Shader> filterShader; // 计算着色器 (Compute Shader)

    // 指向 Persistent Mapping 显存地址的指针
    Point* sharedMappedPtr = nullptr;

    GLuint indirectBuffer; // 用于存储绘制指令
    struct DrawArraysIndirectCommand {
        GLuint count;         // 对应 glDrawArrays 的 count (activePoints)
        GLuint instanceCount; // 1
        GLuint first;        // 0
        GLuint baseInstance; // 0
    };
};