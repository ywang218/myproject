#pragma once
#include <glad/glad.h>   // 确保 GLAD 在前
#include <GLFW/glfw3.h>  // 包含这个才能识别 GLFWwindow
#include "Shader.hpp"
#include "Camera.hpp"
#include "BoxLayer.hpp"
#include "EdgeBoxLayer.hpp"
#include "ObstacleLayer.hpp"
#include "ObstacleEdgeLayer.hpp"
#include "BeltBatch.hpp"
#include "PointCloudLayer.hpp"
#include "data/MockFrameGenerator.hpp"
#include "MockSensorBackend.hpp"
#include <memory>
#include <vector>

// 前向声明，提高编译速度
class PointCloudLayer;

class Renderer {
public:
    Renderer(int width, int height);
    ~Renderer();

    // 对应 animate()
    void run(); 

private:
    GLFWwindow* window;
    int width, height;

    std::unique_ptr<Camera> camera;
    std::unique_ptr<PointCloudLayer> pointCloud;
    std::unique_ptr<MockFrameGenerator> mockFrameGenerator;
    std::unique_ptr<BoxLayer> egoCarLayer;
    std::unique_ptr<EdgeBoxLayer> egoCarEdgeLayer;
    std::unique_ptr<BeltBatch> beltBatch;
    std::unique_ptr<ObstacleLayer> obstacles;
    std::unique_ptr<ObstacleEdgeLayer> edges;
    std::unique_ptr<MockSensorBackend> sensorBackend; // 新成员

    // 障碍物实例化数据的可复用缓冲，避免每帧重新分配 (填充层/边框层共用)
    std::vector<float> obstacleInstanceData;
    // 所有的 Render Items (后期逐个复刻)
    // std::unique_ptr<BeltBatch> beltBatch; 

    void initGLFW();
    void handleInput();
    
    // 对应 updateWorld(data)
    void update(float deltaTime); 
    void render();
};