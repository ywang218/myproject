// #include "data/MockFrameGenerator.hpp"
// #include <cmath>
// #include <ctime>

// MockFrameGenerator::MockFrameGenerator() : sequence(0), egoX(0.0f) {
//     // 初始化车辆
//     for (int i = 0; i < 200; ++i) {
//         Vehicle v;
//         v.id = "veh_" + std::to_string(i);
//         v.x = (rand() % 1000) - 200.0f;
//         v.y = (rand() % 120) - 60.0f;
//         v.heading = ((rand() % 100) / 100.0f - 0.5f) * 0.2f;
//         v.speed = 0.5f + (rand() % 100) / 100.0f;
//         v.l = 4.0f; v.w = 2.0f; v.h = 1.5f;
//         vehicles.push_back(v);
//     }
//     // 静态地图生成逻辑（简化示例）
//     initStaticMap();
// }

// MockFrame MockFrameGenerator::createFrame() {
//     MockFrame frame;
//     frame.timestamp = (double)time(nullptr);
//     frame.seq = ++sequence;

//     // 1. 更新 Ego 
//     egoX += 0.1f; 
//     if (egoX > 700.0f) egoX = 0.0f;
//     frame.ego_pos = glm::vec3(egoX, -60.0f, 0.0f);

//     // 2. 更新并转换车辆为 Polygon
//     for (auto& v : vehicles) {
//         v.x += cos(v.heading) * v.speed;
//         // 边界处理逻辑...
        
//         Polygon p = vehicleToPolygon(v);
//         frame.polygons.push_back(p);
//     }

//     // 3. 加入预生成的静态线段
//     frame.polylines = staticLanes;

//     return frame;
// }

// Polygon MockFrameGenerator::vehicleToPolygon(const Vehicle& v) {
//     Polygon p;
//     p.id = v.id;
//     p.center = glm::vec3(v.x, v.y, v.h / 2.0f);
//     p.heading = v.heading;
    
//     // 计算 4 个角点 (注意：C++ 里的坐标系要和你 Shader 对应，通常 Y 是 Up 或 Z 是 Up)
//     float hl = v.l / 2.0f;
//     float hw = v.w / 2.0f;
//     float c = cos(v.heading);
//     float s = sin(v.heading);

//     float offsets[4][2] = {{hl, hw}, {hl, -hw}, {-hl, -hw}, {-hl, hw}};
//     for(int i=0; i<4; ++i) {
//         float nx = v.x + offsets[i][0] * c - offsets[i][1] * s;
//         float ny = v.y + offsets[i][0] * s + offsets[i][1] * c;
//         p.vertices.push_back(glm::vec3(nx, ny, v.h));
//     }
//     return p;
// }

// // 实现 initStaticMap 函数
// void MockFrameGenerator::initStaticMap() {
//     // 暂时留空，或者先写一点简单的测试数据
//     staticLanes.clear(); 
    
//     // 比如：加一条简单的参考线
//     Polyline refLine;
//     refLine.id = "reference_line";
//     refLine.vertices.push_back(glm::vec3(-200.0f, 0.0f, 0.0f));
//     refLine.vertices.push_back(glm::vec3(800.0f, 0.0f, 0.0f));
//     refLine.style.color = glm::vec4(1.0f, 1.0f, 1.0f, 0.5f); // 白色半透明
    
//     staticLanes.push_back(refLine);
// }

#include "data/MockFrameGenerator.hpp"
#include <cmath>
#include <ctime>
#include <algorithm>
#include <map>
#include <iostream>
#include <cstdint>
#include <vector>

// 1. 极速生成器：Xorshift32
uint32_t xorshift32(uint32_t& state) {
    state ^= state << 13;
    state ^= state >> 17;
    state ^= state << 5;
    return state;
}

// 2. 映射到指定范围 [min, max]
int getFastRandom(uint32_t& state, uint32_t min, uint32_t max) {
    // 计算区间长度（包含端点）
    uint32_t range = max - min + 1; // 10,000,000 - 200,000 + 1 = 9,800,001
    
    // 获取 32 位原始随机数
    uint32_t x = xorshift32(state);
    
    // 核心算法：(x * range) >> 32
    // 使用 uint64_t 防止乘法溢出
    // 这相当于把 x / 2^32 的比例应用到 range 上
    uint32_t scaled = (uint32_t)(((uint64_t)x * range) >> 32);
    
    return (int)(scaled + min);
}

// 辅助工具：将十六进制颜色字符串转换为 glm::vec4 (类似前端的 ColorMap)
glm::vec4 HexToVec4(std::string hex) {
    if (hex[0] == '#') hex = hex.substr(1);
    unsigned int r, g, b, a = 255;
    sscanf(hex.substr(0, 2).c_str(), "%x", &r);
    sscanf(hex.substr(2, 2).c_str(), "%x", &g);
    sscanf(hex.substr(4, 2).c_str(), "%x", &b);
    if (hex.length() == 8) sscanf(hex.substr(6, 2).c_str(), "%x", &a);
    return glm::vec4(r / 255.0f, g / 255.0f, b / 255.0f, a / 255.0f);
}

MockFrameGenerator::MockFrameGenerator() : sequence(0), egoX(0.0f) {
    srand(static_cast<unsigned int>(time(nullptr)));
    
    // 初始化配置
    initStaticMap();
    initVehicles();
    pointCloudBuffer.resize(1000000 * 4);
}

void MockFrameGenerator::initStaticMap() {
    staticLanes.clear();
    
    const int SEGMENTS = 200;
    const float ROAD_LENGTH = 1000.0f;
    const float FORK_START_X = 200.0f;
    const float FORK_END_X = 400.0f;
    const float FORK_OFFSET_Y = 50.0f;
    const float LINE_HEIGHT = -3.0f;
    const float LANE_WIDTH = 3.5f;
    const int LANE_COUNT = 41;

    // 预定义颜色序列
    std::vector<std::string> colorValues = {"#FFFFFFBB", "#FFFF00BB", "#88888844", "#3399FFCC"};
    std::vector<int> lineTypes = {1, 18, 3, 4, 2, 5, 6, 7};

    for (int i = 0; i < LANE_COUNT; ++i) {
        float yBaseOffset = (i - (LANE_COUNT - 1) / 2.0f) * LANE_WIDTH;
        glm::vec4 selectedColor = HexToVec4(colorValues[i % colorValues.size()]);
        int lineType = lineTypes[rand() % lineTypes.size()];
        bool isForkingLane = (i >= 20 && i <= 25);

        auto buildStyle = [&](int type) {
            Style s;
            s.color = selectedColor;
            s.type = type;
            s.semantic_type = "SOLID"; // 简化逻辑
            return s;
        };

        if (isForkingLane) {
            // 1. 主干道部分 (Main)
            Polyline mainLine;
            mainLine.id = "lane_" + std::to_string(i) + "_main";
            mainLine.style = buildStyle(lineType);
            for (int j = 0; j < SEGMENTS; ++j) {
                float x = (j / (float)(SEGMENTS - 1)) * ROAD_LENGTH - 200.0f;
                if (x > FORK_START_X) break;
                float yCurve = std::sin(x * 0.01f) * 20.0f;
                mainLine.vertices.push_back(glm::vec3(x, yBaseOffset + yCurve, LINE_HEIGHT));
            }
            staticLanes.push_back(mainLine);

            // 2. 分叉部分 (Branch)
            Polyline branchLine;
            branchLine.id = "lane_" + std::to_string(i) + "_branch";
            branchLine.style = buildStyle(lineType);
            for (int j = 0; j < SEGMENTS; ++j) {
                float x = (j / (float)(SEGMENTS - 1)) * ROAD_LENGTH - 200.0f;
                if (x < FORK_START_X) continue;
                float yCurve = std::sin(x * 0.01f) * 20.0f;
                float branchY = yBaseOffset;
                if (x < FORK_END_X) {
                    float p = (x - FORK_START_X) / (FORK_END_X - FORK_START_X);
                    branchY += p * FORK_OFFSET_Y;
                } else {
                    branchY += FORK_OFFSET_Y;
                }
                branchLine.vertices.push_back(glm::vec3(x, branchY + yCurve, LINE_HEIGHT));
            }
            staticLanes.push_back(branchLine);
        } else {
            // 普通车道逻辑 (含停止区断开逻辑)
            Polyline line;
            line.id = "lane_" + std::to_string(i);
            line.style = buildStyle(lineType);
            for (int j = 0; j < SEGMENTS; ++j) {
                float x = (j / (float)(SEGMENTS - 1)) * ROAD_LENGTH - 200.0f;
                // 模拟停止区断开 (startX: 220, endX: 235)
                if (x >= 220.0f && x <= 235.0f) continue; 
                
                float yCurve = std::sin(x * 0.01f) * 20.0f;
                line.vertices.push_back(glm::vec3(x, yBaseOffset + yCurve, LINE_HEIGHT));
            }
            staticLanes.push_back(line);
        }
    }
}

void MockFrameGenerator::initVehicles() {
    vehicles.clear();
    for (int i = 0; i < 200; ++i) {
        Vehicle v;
        v.id = "veh_" + std::to_string(i);
        v.x = (float)(rand() % 1000) - 200.0f;
        v.y = (float)(rand() % 140) - 70.0f;
        v.z = 0.0f;
        v.heading = ((rand() % 100) / 100.0f - 0.5f) * 0.2f;
        v.speed = 0.5f + (rand() % 100) / 100.0f;
        v.l = 3.0f + (rand() % 7);
        v.w = 2.0f + (rand() % 2);
        v.h = 1.5f + (rand() % 2);
        vehicles.push_back(v);
    }
}

// void MockFrameGenerator::updatePointCloud(glm::vec3 egoPos) {
//     for (int i = 0; i < 1000000; ++i) {
//         int idx = i * 4;
//         // 在主车周围随机分布 (例如前后 100m，左右 50m)
//         pointCloudBuffer[idx]     = egoPos.x + (rand() % 2000 - 1000) * 0.1f;
//         pointCloudBuffer[idx + 1] = egoPos.y + (rand() % 1000 - 500) * 0.1f;
//         pointCloudBuffer[idx + 2] = (rand() % 100) * 0.05f - 2.0f; // 地面附近
//         pointCloudBuffer[idx + 3] = 1.0f; // 强度/颜色占位
//     }
// }
uint32_t start = 600000;
MockFrame MockFrameGenerator::createFrame() {
    MockFrame frame;
    frame.timestamp = (double)time(nullptr);
    frame.seq = ++sequence;

    // 1. Ego 逻辑
    egoX += 1.0f;
    if (egoX > 700.0f) egoX = 0.0f;
    frame.ego_pos = glm::vec3(egoX, -60.0f, 0.0f);

    // 2. 车辆更新与多边形转换
    frame.polygons.reserve(vehicles.size()); // 预留空间，避免 push_back 反复扩容
    for (auto& v : vehicles) {
        v.x += std::cos(v.heading) * v.speed;
        if (v.x > 800.0f) v.x = -200.0f;
        frame.polygons.push_back(vehicleToPolygon(v));
    }

    // 3. 静态地图
    frame.polylines = staticLanes;
    
    // 4. point cloud
    // updatePointCloud(frame.ego_pos);
    frame.point_data = pointCloudBuffer.data();
    frame.point_count = getFastRandom(start, 200000, 10000000);  // 200000 + random() * 9800000; //1000000;
    return frame;
}

Polygon MockFrameGenerator::vehicleToPolygon(const Vehicle& v) {
    Polygon p;
    p.id = v.id;
    p.center = glm::vec3(v.x, v.y, v.h / 2.0f);
    p.heading = v.heading;
    p.size = glm::vec3(v.l, v.w, v.h);
    p.style.color = HexToVec4("#32CD32");

    // 注意：障碍物渲染只用 center/size/heading 算 model 矩阵，
    // 拾取只用 center/size —— 4 个角点 (p.vertices) 全程无人读取。
    // 故不再计算，省去每帧 200 次 vector 堆分配。
    return p;
}