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
#include "Point.hpp"

class MockSensorBackend {
public:
    MockSensorBackend(int count) : pointCount(count), running(false) {
        // 预分配索引数组，用于多线程并行任务分配
        indices.resize(pointCount);
        std::iota(indices.begin(), indices.end(), 0);
    }
    
    ~MockSensorBackend() { stop(); }

    void start(Point* sharedMemoryPtr) {
        if (running || !sharedMemoryPtr) return;
        targetPtr = sharedMemoryPtr;
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

private:
    void updateLoop() {
        while (running) {
            auto startTime = std::chrono::high_resolution_clock::now();

            glm::vec3 ego;
            {
                std::lock_guard<std::mutex> lock(mtx);
                ego = currentEgoPos;
            }

            // --- 核心优化：并行生成数据 (Parallel Data Generation) ---
            // std::execution::par_unseq 允许编译器使用多线程 + SIMD 指令集加速
            std::for_each(std::execution::par_unseq, indices.begin(), indices.end(), [&](int i) {
                // 使用 thread_local 保证多线程下随机数生成的独立性和高性能，避免锁竞争
                static thread_local std::mt19937 gen(std::random_device{}());
                static thread_local std::uniform_real_distribution<float> dis(-1.0f, 1.0f);

                float rx = dis(gen);
                float ry = dis(gen);
                float rz = dis(gen);

                // 模拟真实的 backend 数据计算
                targetPtr[i].pos = glm::vec4(
                    ego.x + rx * 100.0f, 
                    ego.y + ry * 50.0f, 
                    rz * 2.0f, 
                    1.0f
                );
                
                // 模拟强度变化数据   !IMPORTANT: targetPtr is sharedMappedPtr who be passed in and targetPtr 就是指向 inputSSBO 的 CPU 端映射
                targetPtr[i].color = glm::vec4(0.2f, 0.4f + rz * 0.2f, 1.0f, 0.8f);
            });

            // 维持 10Hz 更新率
            auto endTime = std::chrono::high_resolution_clock::now();
            auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime);
            auto sleepTime = std::chrono::milliseconds(100) - elapsed;

            if (sleepTime.count() > 0) {
                std::this_thread::sleep_for(sleepTime);
            }
        }
    }

    int pointCount;
    Point* targetPtr = nullptr;
    std::thread workerThread;
    std::atomic<bool> running;
    
    glm::vec3 currentEgoPos{0.0f};
    std::mutex mtx;

    // 辅助索引数组，用于并行分发任务
    std::vector<int> indices;
};