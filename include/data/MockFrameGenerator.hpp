// #pragma once
// #include <vector>
// #include <string>
// #include <glm/glm.hpp>
// #include <cstdint>

// // 基础颜色和语义定义
// struct Style {
//     glm::vec4 color;
//     int type;
//     std::string semantic_type;
// };

// struct Polyline {
//     std::string id;
//     std::vector<glm::vec3> vertices;
//     float width;
//     Style style;
// };

// struct Polygon {
//     std::string id;
//     std::vector<glm::vec3> vertices; // 4个角点
//     glm::vec3 center;
//     glm::vec3 size;
//     float heading;
//     Style style;
// };

// struct Vehicle {
//     std::string id;
//     float x, y, z;
//     float heading;
//     float speed;
//     float l, w, h;
// };

// struct MockFrame {
//     double timestamp;
//     uint32_t seq;
//     glm::vec3 ego_pos;
//     std::vector<Polyline> polylines;
//     std::vector<Polygon> polygons;
// };

// class MockFrameGenerator {
// public:
//     // 1. 构造函数：初始化数据
//     MockFrameGenerator();

//     // 2. 核心功能：生成一帧数据数据
//     // 返回 MockFrame 对象。注意：在 C++17 中这很高效（返回值优化 RVO）
//     MockFrame createFrame();

// private:
//     // 3. 成员变量：这些数据在 Generator 的生命周期内一直存在
//     uint32_t sequence;
//     float egoX;
//     std::vector<Vehicle> vehicles;
//     std::vector<Polyline> staticLanes;

//     // 4. 辅助函数：内部逻辑封装
//     void initStaticMap();
//     Polygon vehicleToPolygon(const Vehicle& v);
// };


#pragma once
#include <vector>
#include <string>
#include <glm/glm.hpp>
#include <cstdint>

// 1. 基础结构体定义 (确保在类定义之前)
struct Style {
    glm::vec4 color;
    int type;
    std::string semantic_type;
};

struct Polyline {
    std::string id;
    std::vector<glm::vec3> vertices;
    float width;
    Style style;
};

struct Polygon {
    std::string id;
    std::vector<glm::vec3> vertices; // 4个角点
    glm::vec3 center;
    glm::vec3 size;                  // 对应 JS 的 [length, width, height]
    float heading;
    Style style;
};

struct Vehicle {
    std::string id;
    float x, y, z;
    float heading;
    float speed;
    float l, w, h;
};

struct MockFrame {
    double timestamp;
    uint32_t seq;
    glm::vec3 ego_pos;
    std::vector<Polyline> polylines;
    std::vector<Polygon> polygons;
    // point cloud in future
    const float* point_data; 
    size_t point_count;
};

// 2. 类定义
class MockFrameGenerator {
public:
    MockFrameGenerator();
    
    // 生成一帧完整数据
    MockFrame createFrame();

private:
    // 成员变量
    uint32_t sequence;
    float egoX;
    std::vector<Vehicle> vehicles;      // 存储原始车辆状态
    std::vector<Polyline> staticLanes;  // 存储预生成的静态地图
    std::vector<float> pointCloudBuffer; // 存储 100w 个点 (x, y, z, w)
    void updatePointCloud(glm::vec3 egoPos);

    // 内部辅助函数 (必须在这里声明，.cpp 才能实现)
    void initStaticMap();               // 初始化分叉路口和车道
    void initVehicles();                // 初始化 200 辆车
    Polygon vehicleToPolygon(const Vehicle& v); // 车辆逻辑转多边形渲染数据
};