#pragma once
#include <glad/glad.h>   // 确保 GLAD 在前
#include <GLFW/glfw3.h>  // 包含这个才能识别 GLFWwindow
#include "Shader.hpp"
#include "Camera.hpp"
#include <memory>

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
    // 所有的 Render Items (后期逐个复刻)
    // std::unique_ptr<BeltBatch> beltBatch; 

    void initGLFW();
    void handleInput();
    
    // 对应 updateWorld(data)
    void update(float deltaTime); 
    void render();
};