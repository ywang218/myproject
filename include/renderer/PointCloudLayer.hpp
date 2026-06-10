
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
#include <vector>
#include <deque>
#include <mutex>
#include "renderer/Shader.hpp"
#include "Point.hpp"

class PointCloudLayer {
public:
    PointCloudLayer(int count);
    ~PointCloudLayer();

    // ===== 三缓冲生产者/消费者接口 =====
    // [worker 线程, 不含 GL] 取一块空闲输入缓冲的映射指针写入；无空闲返回 nullptr (本帧丢弃)。
    Point* acquireWriteBuffer(int& outIndex);
    // [worker 线程, 不含 GL] 写完后发布：缓冲索引、生成时用的 egoPos、本帧实际点数。
    void publishFrame(int index, const glm::vec3& ego, uint32_t count);

    // [渲染线程] 回收 GPU 已读完的缓冲，并消费最新就绪帧做 compute 剔除；剔除了返回 true。
    bool consumeAndFilter();
    // [渲染线程] 每帧调用：用上一次剔除的结果做 indirect draw；egoWorld 为本帧实时车辆位置
    void draw(const glm::mat4& view, const glm::mat4& projection, const glm::vec3& egoWorld);

    // --- 性能探针 (第 0 步：测量) ---
    double   getComputeMs() const { return lastComputeMs; } // 最近一次 compute 剔除的 GPU 耗时
    double   getDrawMs()    const { return lastDrawMs; }    // 最近一次 indirect draw 的 GPU 耗时
    unsigned getFilterRuns() const { return filterRuns; }   // 自上次清零以来 runFilter 调用次数
    void     resetFilterRuns() { filterRuns = 0; }
    unsigned getLastInputCount() const { return lastInputCount; } // 最近一次剔除的输入点数 (worker 发布值)
    // 回读当前 indirect buffer 里的实际绘制点数 (会触发同步，建议每秒调一次)
    uint32_t readDrawnCount();

private:
    // [渲染线程内部] 执行 compute 剔除：绑定 inputSSBO[index]、按 count 设 uTotalPoints、
    // dispatch、屏障，并对该缓冲打 fence。
    void runFilter(int index, const glm::vec3& egoPos, uint32_t count);

    // ╔══════════════════════════════════════════════════════════════════╗
    // ║ 【手搓 三缓冲 + FENCE】之一：数据结构                                  ║
    // ║ inputSSBO/mappedPtr/fences 三件套 + 下面的 freeList/readyList/poolMtx ║
    // ║ 共同构成「worker 写、GPU 读」的无锁交接 + GPU 读取完成同步。            ║
    // ╚══════════════════════════════════════════════════════════════════╝
    static const int NUM_BUFFERS = 3; // 输入三缓冲

    int pointCount; // 单个输入缓冲的容量 (最大点数)

    // --- 资源句柄 ---
    GLuint vao;
    GLuint inputSSBO[NUM_BUFFERS];    // ★三缓冲：三块 Persistent Mapping 输入缓冲
    Point* mappedPtr[NUM_BUFFERS] = {nullptr, nullptr, nullptr}; // ★三缓冲：各自的 CPU 映射指针
    GLsync fences[NUM_BUFFERS] = {nullptr, nullptr, nullptr};    // ★FENCE：守护各缓冲的 GPU 读取完成
    GLuint outputSSBO;   // 过滤后的紧凑缓冲区
    GLuint atomicBuffer; // 存储有效点数的计数器
    GLint uEgoPosLoc, uTotalPointsLoc;

    std::unique_ptr<Shader> renderShader; // 顶点/片元着色器
    std::unique_ptr<Shader> filterShader; // 计算着色器 (Compute Shader)

    GLuint indirectBuffer; // 用于存储绘制指令

    // --- 三缓冲调度状态 (worker 与渲染线程共享，poolMtx 保护) ---
    std::mutex poolMtx;
    std::vector<int> freeList;            // 可写的空闲缓冲索引
    struct ReadyFrame { int index; glm::vec3 ego; uint32_t count; };
    std::deque<ReadyFrame> readyList;     // worker 已写好、待消费的帧

    // --- 性能探针：GPU 计时 query (单 query + pending 标志，读上一次结果避免 stall) ---
    GLuint   queryCompute = 0; bool qcPending = false;
    GLuint   queryDraw    = 0; bool qdPending = false;
    double   lastComputeMs = 0.0;
    double   lastDrawMs = 0.0;
    unsigned filterRuns = 0;
    unsigned lastInputCount = 0; // 最近一次剔除用的输入点数

    struct DrawArraysIndirectCommand {
        GLuint count;         // 对应 glDrawArrays 的 count (activePoints)
        GLuint instanceCount; // 1
        GLuint first;        // 0
        GLuint baseInstance; // 0
    };
};