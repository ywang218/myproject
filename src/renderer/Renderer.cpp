#include "renderer/Renderer.hpp"
// #include "renderer/PointCloudLayer.hpp"
#include "data/MockFrameGenerator.hpp"
#include "renderer/BoxLayer.hpp"
#include "renderer/ObstacleLayer.hpp"
#include "renderer/ObstacleEdgeLayer.hpp"
#include <iostream>
#include <iomanip>

// 全局/静态变量用于回调
static Camera* g_camera = nullptr;
static bool leftButtonPressed = false;
static double lastX, lastY;
static bool firstMouse = true;

// 简单输入回调实现
void mouse_callback(GLFWwindow* window, double xpos, double ypos) {
    if (firstMouse) { lastX = xpos; lastY = ypos; firstMouse = false; }
    if (leftButtonPressed && g_camera) {
        // 这里可以实现类似你 CameraController.js 的旋转逻辑
    }
    lastX = xpos; lastY = ypos;
}

Renderer::Renderer(int w, int h) : width(w), height(h) {
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    window = glfwCreateWindow(width, height, "AutoDrive Engine C++", NULL, NULL);
    if (!window) { glfwTerminate(); }
    
    glfwMakeContextCurrent(window);
    gladLoadGLLoader((GLADloadproc)glfwGetProcAddress);

    glEnable(GL_DEPTH_TEST);
    glEnable(GL_PROGRAM_POINT_SIZE);
    // --- 新增：开启混合 (Alpha Blending) ---
    glEnable(GL_BLEND);
    // 设置混合公式：结果 = (源颜色 * 源Alpha) + (目标颜色 * (1 - 源Alpha))
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    // ------------------------------------
    glfwSwapInterval(0); // 关闭垂直同步

    // 初始化相机和图层
    camera = std::make_unique<Camera>(glm::vec3(0, 30, 60));
    g_camera = camera.get(); // 供回调使用

    pointCloud = std::make_unique<PointCloudLayer>(10000000);
    egoCarLayer = std::make_unique<BoxLayer>();
    egoCarEdgeLayer = std::make_unique<EdgeBoxLayer>();
    beltBatch = std::make_unique<BeltBatch>(200000);
    obstacles = std::make_unique<ObstacleLayer>(300);
    edges = std::make_unique<ObstacleEdgeLayer>(300);
    sensorBackend = std::make_unique<MockSensorBackend>(1000000);

    // 3. 【关键：串联逻辑】
    // 获取点云图层映射好的 GPU 内存指针
    Point* gpuMemoryPtr = pointCloud->getMappedPointer();

    // 把指针交给后端，并启动后台生成线程
    if (gpuMemoryPtr) {
        sensorBackend->start(gpuMemoryPtr);
    }
}

Renderer::~Renderer() {
    glfwTerminate();
}

void Renderer::run() {
    MockFrameGenerator generator;
    int frameCount = 0;
    double lastTime = glfwGetTime();
    int nbFrames = 0;

    // 确保这里关闭了垂直同步
    glfwSwapInterval(0);
    while (!glfwWindowShouldClose(window)) {
        float currentTime = (float)glfwGetTime();
        nbFrames++;
        
        // 如果距离上次更新超过 1 秒
        if (currentTime - lastTime >= 1.0) {
            double fps = double(nbFrames) / (currentTime - lastTime);
            
            // 构造新的标题字符串
            // 使用 (int)fps 让数字看起来更整洁，不跳动
            std::string newTitle = "AutoDrive Engine C++ | FPS: " + std::to_string((int)fps);
            
            // 更新窗口标题
            glfwSetWindowTitle(window, newTitle.c_str());

            // 重置计数器
            nbFrames = 0;
            lastTime = currentTime;
        } 

        MockFrame frame = generator.createFrame();
        // std::cout << "point cont" << frame.point_count << std::endl;

        if (frameCount % 60 == 0) {
            std::cout << "\033[1;32m[Frame Info]\033[0m " 
                      << "Seq: " << frame.seq 
                      << " | EgoX: " << std::fixed << std::setprecision(2) << frame.ego_pos.x
                      << " | Vehicles: " << frame.polygons.size()
                      << " | Lines: " << frame.polylines.size() << std::endl;
        }
        frameCount++;

        // // 1. Update (数据处理)
        // pointCloud->update(currentTime);

        // 2. Render (绘制)
        glClearColor(0.01f, 0.01f, 0.03f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // 2. 更新相机逻辑 (参数：位置, 偏航角, 跟随距离, 高度, 平滑系数)
        camera->updateFollow(frame.ego_pos, 0.0f, 25.0f, 15.0f, 0.15f);
        glm::mat4 view = camera->GetViewMatrix();
        glm::mat4 proj = camera->GetProjectionMatrix((float)width, (float)height);

        sensorBackend->setEgoPosition(frame.ego_pos);  
        // pointCloud->render(view, proj);
        egoCarLayer->render(frame.ego_pos, 0.0f, glm::vec3(4.0f, 2.0f, 1.5f), view, proj);
        egoCarEdgeLayer->render(frame.ego_pos, 0.0f, glm::vec3(4.0f, 2.0f, 1.5f), view, proj);
        
        if(frame.point_count > 5000000) {
            pointCloud->setLOD(10);
        } else if(frame.point_count > 2000000 && frame.point_count <= 5000000) {
            pointCloud->setLOD(5); 
        } else {
            pointCloud->setLOD(1);
        }
        
        pointCloud->render(view, proj); 

        beltBatch->begin();
        for (auto& line : frame.polylines) {
            bool isDashed = (line.style.type == 3 || line.style.type == 4); // 简化逻辑
            glm::vec3 color = glm::vec3(line.style.color); // MockFrameGenerator 已经转好了 vec4
            
            // 参考 JS AMap.js 的双线逻辑
            if (line.style.type == 2) { // 假设 2 是双线
                beltBatch->writeBelt(line.vertices, -0.3f, 0.5f, false, color);
                beltBatch->writeBelt(line.vertices,  0.3f, 0.5f, false, color);
            } else {
                beltBatch->writeBelt(line.vertices, 0.0f, 0.5f, isDashed, color);
            }
        }
        // 渲染
        
        beltBatch->render(view, proj);

        obstacles->updateData(frame.polygons); // 传入 Mock 数据
        obstacles->render(view, proj);         // 绘制

        edges->updateData(frame.polygons);
        edges->render(view, proj);
        glfwSwapBuffers(window);
        glfwPollEvents();
    }
}