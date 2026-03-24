
// #include "renderer/PointCloudLayer.hpp"
// #include <glm/gtc/type_ptr.hpp>

// PointCloudLayer::PointCloudLayer(int count) : pointCount(count), keepRunning(true) {
//     // 1. 加载 Shader
//     renderShader = std::make_unique<Shader>("shaders/point_cloud.vert", "shaders/point_cloud.frag");
//     computeShader = std::make_unique<Shader>("shaders/point_cloud.comp");

//     // 2. 初始化 Persistent Mapping SSBO
//     glGenBuffers(1, &ssbo);
//     glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssbo);
    
//     GLbitfield flags = GL_MAP_WRITE_BIT | GL_MAP_PERSISTENT_BIT | GL_MAP_COHERENT_BIT;
//     glBufferStorage(GL_SHADER_STORAGE_BUFFER, pointCount * sizeof(Point), nullptr, flags);
//     sharedMappedPtr = (Point*)glMapBufferRange(GL_SHADER_STORAGE_BUFFER, 0, pointCount * sizeof(Point), flags);
    
//     // 绑定到插槽 0
//     glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, ssbo);

//     // 3. 初始化 VAO (点云只需要一个空的 VAO)
//     glGenVertexArrays(1, &vao);

//     // 4. 启动生产者线程
//     producerThread = std::thread(&PointCloudLayer::dataProducerWorker, this);
// }

// PointCloudLayer::~PointCloudLayer() {
//     keepRunning = false;
//     if (producerThread.joinable()) producerThread.join();
    
//     glUnmapBuffer(GL_SHADER_STORAGE_BUFFER);
//     glDeleteBuffers(1, &ssbo);
//     glDeleteVertexArrays(1, &vao);
// }

// void PointCloudLayer::dataProducerWorker() {
//     // while (keepRunning) {
//         for (int i = 0; i < pointCount; i++) {
//             sharedMappedPtr[i].pos[0] = 10.0f + (float)(rand() % 2000) / 40.0f;
//             sharedMappedPtr[i].pos[1] = (float)(rand() % 628) / 100.0f;
//             sharedMappedPtr[i].pos[2] = (float)(rand() % 100 - 50) / 100.0f;
//             sharedMappedPtr[i].color[0] = 0.5f + (float)(rand() % 50) / 100.0f;
//         }
//         std::this_thread::yield(); 
//     // }
// }

// void PointCloudLayer::update(float currentTime) {
//     computeShader->use();
//     glUniform1f(glGetUniformLocation(computeShader->ID, "uTime"), currentTime);
//     glDispatchCompute((pointCount + 255) / 256, 1, 1);
//     glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
// }

// void PointCloudLayer::render(const glm::mat4& view, const glm::mat4& projection) {
//     renderShader->use();
//     renderShader->setMat4("uView", view);
//     renderShader->setMat4("uProjection", projection);

//     glBindVertexArray(vao);
//     glDrawArrays(GL_POINTS, 0, pointCount);
// }

#include "renderer/PointCloudLayer.hpp"
#include <glm/gtc/type_ptr.hpp>

PointCloudLayer::PointCloudLayer(int count) : pointCount(count) {
    // 1. 加载 Shader
    // 注意：暂时去掉了 computeShader，因为我们现在改用 CPU 后端模拟。
    // 如果以后要用 GPU 生成，再加回来。
    renderShader = std::make_unique<Shader>("shaders/point_cloud.vert", "shaders/point_cloud.frag");

    // 2. 初始化 Persistent Mapping SSBO
    glGenBuffers(1, &ssbo);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssbo);
    
    // 设置标志位：
    // GL_MAP_WRITE_BIT: 我们要写它
    // GL_MAP_PERSISTENT_BIT: 映射后指针永久有效，不用每帧 map/unmap
    // GL_MAP_COHERENT_BIT: 写入后立即对 GPU 可见，无需手动执行 flush
    GLbitfield flags = GL_MAP_WRITE_BIT | GL_MAP_PERSISTENT_BIT | GL_MAP_COHERENT_BIT;
    
    // 创建不可变存储 (Immutable Storage)
    glBufferStorage(GL_SHADER_STORAGE_BUFFER, pointCount * sizeof(Point), nullptr, flags);
    
    // 获取映射指针
    sharedMappedPtr = (Point*)glMapBufferRange(GL_SHADER_STORAGE_BUFFER, 0, 
                                               pointCount * sizeof(Point), flags);
    
    // 绑定到插槽 0 (Shader 里的 binding = 0 对应这里)
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, ssbo);

    // 3. 初始化 VAO
    glGenVertexArrays(1, &vao);
}

PointCloudLayer::~PointCloudLayer() {
    if (sharedMappedPtr) {
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssbo);
        glUnmapBuffer(GL_SHADER_STORAGE_BUFFER);
    }
    glDeleteBuffers(1, &ssbo);
    glDeleteVertexArrays(1, &vao);
}

void PointCloudLayer::render(const glm::mat4& view, const glm::mat4& projection) {
    if (!sharedMappedPtr) return;

    renderShader->use();
    renderShader->setMat4("uView", view);
    renderShader->setMat4("uProjection", projection);

    // 点云绘制只需要绑定 VAO
    // 数据已经在渲染之前由 Backend 线程静默写入 SSBO 了
    glBindVertexArray(vao);
    glDrawArrays(GL_POINTS, 0, pointCount);
}