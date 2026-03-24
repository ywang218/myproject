// #ifndef POINT_CLOUD_LAYER_H
// #define POINT_CLOUD_LAYER_H

// #include <glad/glad.h>
// #include <glm/glm.hpp>
// #include <vector>
// #include <thread>
// #include <atomic>
// #include <memory>
// #include "Shader.hpp"

// // struct Point { 
// //     alignas(16) float pos[4]; 
// //     alignas(16) float color[4]; 
// // };

// class PointCloudLayer {
// public:
//     PointCloudLayer(int count);
//     ~PointCloudLayer();

//     // 更新逻辑（计算着色器等）
//     void update(float currentTime);
    
//     // 渲染逻辑
//     void render(const glm::mat4& view, const glm::mat4& projection);

// private:
//     int pointCount;
//     GLuint ssbo, vao;
//     std::unique_ptr<Shader> renderShader;
//     std::unique_ptr<Shader> computeShader;

//     // 多线程生产者相关
//     std::atomic<bool> keepRunning;
//     std::thread producerThread;
//     Point* sharedMappedPtr = nullptr;

//     // 内部线程函数
//     void dataProducerWorker();
// };

// #endif



#pragma once
#include <glad/glad.h>
#include <glm/glm.hpp>
#include <memory>
#include "renderer/Shader.hpp"
#include "Point.hpp"

// 结构体定义，确保与 Backend 一致
// struct Point {
//     glm::vec4 pos;   // x, y, z, intensity
//     glm::vec4 color; // r, g, b, a
// };

class PointCloudLayer {
public:
    PointCloudLayer(int count);
    ~PointCloudLayer();

    // 关键：暴露这个指针给 Backend，让 Backend 能直接写显存
    Point* getMappedPointer() { return sharedMappedPtr; }

    // 渲染逻辑
    void render(const glm::mat4& view, const glm::mat4& projection);

private:
    int pointCount;
    GLuint ssbo, vao;
    std::unique_ptr<Shader> renderShader;

    // 指向 Persistent Mapping 显存地址的指针
    Point* sharedMappedPtr = nullptr;
};