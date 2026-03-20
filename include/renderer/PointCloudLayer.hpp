#ifndef POINT_CLOUD_LAYER_H
#define POINT_CLOUD_LAYER_H

#include <glad/glad.h>
#include <glm/glm.hpp>
#include <vector>
#include <thread>
#include <atomic>
#include <memory>
#include "Shader.hpp"

struct Point { 
    alignas(16) float pos[4]; 
    alignas(16) float color[4]; 
};

class PointCloudLayer {
public:
    PointCloudLayer(int count);
    ~PointCloudLayer();

    // 更新逻辑（计算着色器等）
    void update(float currentTime);
    
    // 渲染逻辑
    void render(const glm::mat4& view, const glm::mat4& projection);

private:
    int pointCount;
    GLuint ssbo, vao;
    std::unique_ptr<Shader> renderShader;
    std::unique_ptr<Shader> computeShader;

    // 多线程生产者相关
    std::atomic<bool> keepRunning;
    std::thread producerThread;
    Point* sharedMappedPtr = nullptr;

    // 内部线程函数
    void dataProducerWorker();
};

#endif