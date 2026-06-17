// #pragma once
// #include <vector>
// #include <thread>
// #include <atomic>
// #include <mutex>
// #include <chrono>
// #include "Point.hpp"

// class MockSensorBackend {
// public:
//     MockSensorBackend(int count) : pointCount(count), running(false) {}
//     ~MockSensorBackend() { stop(); }

//     // 启动后台线程
//     void start(Point* sharedMemoryPtr) {
//         if (running) return;
//         targetPtr = sharedMemoryPtr;
//         running = true;
//         workerThread = std::thread(&MockSensorBackend::updateLoop, this);
//     }

//     void stop() {
//         running = false;
//         if (workerThread.joinable()) workerThread.join();
//     }

//     // 更新当前的自车位置，模拟传感器跟随主车
//     void setEgoPosition(glm::vec3 pos) {
//         std::cout << "pos set, start simulate points cloud" << pos.x << std::endl;
//         std::lock_guard<std::mutex> lock(mtx);
//         currentEgoPos = pos;
//     }

// private:
//     void updateLoop() {
//         while (running) {
//             auto startTime = std::chrono::high_resolution_clock::now();

//             glm::vec3 ego;
//             {
//                 std::lock_guard<std::mutex> lock(mtx);
//                 ego = currentEgoPos;
//             }

//             // --- 核心：模拟后端算法生成 100 万个点 ---
//             for (int i = 0; i < pointCount; ++i) {
//                 // 在主车周围生成随机点
//                 float x = ego.x + (rand() % 2000 - 1000) * 0.1f;
//                 float y = ego.y + (rand() % 1000 - 500) * 0.1f;
//                 float z = (rand() % 200) * 0.02f - 2.0f; // 地面附近
                
//                 targetPtr[i].pos = glm::vec4(x, y, z, 1.0f);
//                 targetPtr[i].color = glm::vec4(0.2f, 0.6f, 1.0f, 0.8f); // 科技蓝色
//             }
//             // std::cout << "target len: " << targetPtr.size() << std::endl;

//             // 模拟 10Hz (100ms 一帧)
//             auto endTime = std::chrono::high_resolution_clock::now();
//             auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime);
//             auto sleepTime = std::chrono::milliseconds(100) - elapsed;

//             if (sleepTime.count() > 0) {
//                 std::this_thread::sleep_for(sleepTime);
//             }
//         }
//     }

//     int pointCount;
//     Point* targetPtr = nullptr; // 指向 OpenGL 映射的显存地址
//     std::thread workerThread;
//     std::atomic<bool> running;
    
//     glm::vec3 currentEgoPos{0.0f};
//     std::mutex mtx;
// };


#pragma once
#include <vector>
#include <thread>
#include <atomic>
#include <mutex>
#include <chrono>
#include <random>
#include <algorithm>
#include <execution> // 需要 C++17 支持并行算法
#include <numeric>   // std::iota
#include <cmath>     // sin/cos/tan (有组织扫描生成)
#include "Point.hpp"
#include "renderer/PointCloudLayer.hpp" // 三缓冲池接口

class MockSensorBackend {
public:
    MockSensorBackend(int count) : pointCount(count), running(false), countRng(12345u) {
        // 预分配索引数组，用于多线程并行任务分配
        indices.resize(pointCount);
        std::iota(indices.begin(), indices.end(), 0);
    }

    ~MockSensorBackend() { stop(); }

    // 接入点云图层的三缓冲池；worker 从池里取缓冲写、写完发布。
    void start(PointCloudLayer* poolPtr) {
        if (running || !poolPtr) return;
        pool = poolPtr;
        running = true;
        workerThread = std::thread(&MockSensorBackend::updateLoop, this);
    }

    void stop() {
        running = false;
        if (workerThread.joinable()) workerThread.join();
    }

    void setEgoPosition(glm::vec3 pos) {
        std::lock_guard<std::mutex> lock(mtx);
        currentEgoPos = pos;
    }

    // 开启「有组织」扫描生成 (gradient.comp 算曲率所需)。
    // 点按 idx = ring*width + col 排布 → 同 ring 的 azimuth 邻居就是 idx±k。
    // rings*width 必须 ≤ pointCount(缓冲容量)。默认 128×1024 = 131072 点。
    void setOrganized(bool on, int rings = 128, int width = 1024) {
        organized_ = on; organizedRings_ = rings; organizedWidth_ = width;
    }

private:
    void updateLoop() {
        while (running) {
            auto startTime = std::chrono::high_resolution_clock::now();

            glm::vec3 ego;
            {
                std::lock_guard<std::mutex> lock(mtx);
                ego = currentEgoPos;
            }

            // 1. 从池里取一块空闲输入缓冲 (无 GL 调用，纯写映射内存)
            int idx = -1;
            Point* dst = pool->acquireWriteBuffer(idx);
            if (!dst) {
                // 无空闲缓冲 (渲染线程暂未回收完)，丢弃本帧稍后重试
                std::this_thread::sleep_for(std::chrono::milliseconds(2));
                continue;
            }

            // 2~3. 生成本帧点云。两种模式：
            uint32_t count;
            if (organized_) {
                // 【有组织扫描】idx = ring*W + col，供 gradient.comp 用 idx±k 取真邻居。
                count = generateOrganized(dst);
            } else {
                // 【随机散点 (原行为)】每帧点数可变 [300w, 500w]，上限不超缓冲容量 pointCount。
                const uint32_t kMin = 3000000u;
                const uint32_t kMax = (pointCount < 5000000) ? (uint32_t)pointCount : 5000000u;
                count = kMin + (uint32_t)(countRng() % (uint32_t)(kMax - kMin + 1));

                std::for_each(std::execution::par_unseq, indices.begin(), indices.begin() + count, [&](int i) {
                    static thread_local std::mt19937 gen(std::random_device{}());
                    static thread_local std::uniform_real_distribution<float> dis(-1.0f, 1.0f);

                    float rx = dis(gen);
                    float ry = dis(gen);
                    float rz = dis(gen);

                    // 传感器/ego 相对坐标 (不烘 ego)。渲染时每帧用实时车辆位姿摆进世界，
                    // 这样点云平滑跟车，只有图案按 10Hz 刷新——与真实激光雷达可视化一致。
                    dst[i].pos = glm::vec4(
                        rx * 100.0f,
                        ry * 50.0f,
                        rz * 2.0f,
                        1.0f
                    );
                    dst[i].color = glm::vec4(0.2f, 0.4f + rz * 0.2f, 1.0f, 0.8f);
                });
            }

            // 4. 发布该帧 (索引 / egoPos / 实际点数)
            pool->publishFrame(idx, ego, count);

            // 维持 10Hz 更新率
            auto endTime = std::chrono::high_resolution_clock::now();
            auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime);
            auto sleepTime = std::chrono::milliseconds(100) - elapsed;

            if (sleepTime.count() > 0) {
                std::this_thread::sleep_for(sleepTime);
            }
        }
    }

    // 生成一帧「有组织」扫描点云：idx = ring*W + col，返回点数 = RINGS*W (≤pointCount)。
    // pos.w 先写入「原始强度」(墙强、地面弱)——gradient.comp 会据邻域曲率把它覆写成 score。
    // 合成场景 = 地面 + 两道墙，制造真实深度突变(边)，让曲率计算有意义。
    // ★ 坐标轴约定此处为 y=上(elev 抬升 y)；如与你的相机不符，调换 y/z 即可。
    uint32_t generateOrganized(Point* dst) {
        const int RINGS = organizedRings_;
        const int W = organizedWidth_;
        uint32_t count = (uint32_t)(RINGS * W);
        if (count > (uint32_t)pointCount) count = (uint32_t)pointCount; // 不超缓冲容量

        const float DEG = 3.14159265f / 180.0f;
        const float elevMin = -15.0f * DEG, elevMax = 15.0f * DEG; // 垂直视场 ±15°

        std::for_each(std::execution::par_unseq, indices.begin(), indices.begin() + count, [&](int i) {
            int ring = i / W;
            int col  = i % W;
            float elev = elevMin + (elevMax - elevMin) * (RINGS > 1 ? ring / float(RINGS - 1) : 0.0f);
            float az   = 6.2831853f * (col / float(W)); // 0..2π

            // 合成「距离」：地面随俯仰角延伸 + 两道墙(固定方位扇区) → 深度突变=边
            float ground = (std::fabs(elev) > 1e-3f) ? (1.8f / std::tan(std::fabs(elev))) : 150.0f;
            float wall = 1e9f;
            if (az > 0.6f && az < 1.2f) wall = 12.0f; // 右前一道墙
            if (az > 3.5f && az < 4.0f) wall = 25.0f; // 左后一道墙
            float r = std::min(std::min(ground, wall), 150.0f);

            float ce = std::cos(elev);
            float x = r * ce * std::cos(az);
            float y = r * std::sin(elev);
            float z = r * ce * std::sin(az);
            float intensity = (wall < ground) ? 0.9f : 0.3f; // 墙反射强、地面弱

            dst[i].pos   = glm::vec4(x, y, z, intensity); // ★ pos.w = 原始强度
            dst[i].color = glm::vec4(0.2f, 0.6f, 1.0f, 0.8f);
        });
        return count;
    }

    int pointCount;
    PointCloudLayer* pool = nullptr;
    std::thread workerThread;
    std::atomic<bool> running;

    glm::vec3 currentEgoPos{0.0f};
    std::mutex mtx;

    std::mt19937 countRng;        // 每帧点数随机 (仅 worker 线程使用)
    std::vector<int> indices;     // 并行分发用的索引数组

    // 有组织扫描生成开关与分辨率 (gradient.comp 所需)
    bool organized_ = false;
    int  organizedRings_ = 128;
    int  organizedWidth_ = 1024;
};