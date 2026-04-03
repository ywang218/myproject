// #include "renderer/Renderer.hpp"
// #include "renderer/PointCloudLayer.hpp"
// #include "data/MockFrameGenerator.hpp"
// #include "renderer/BoxLayer.hpp"
// #include "renderer/ObstacleLayer.hpp"
// #include "renderer/ObstacleEdgeLayer.hpp"
// #include "imgui.h"
// #include "imgui_impl_glfw.h"
// #include "imgui_impl_opengl3.h"
// #include <iostream>
// #include <iomanip>

// // 全局/静态变量用于回调
// static Camera* g_camera = nullptr;
// static bool leftButtonPressed = false;
// static double lastX, lastY;
// static bool firstMouse = true;

// // 简单输入回调实现
// void mouse_callback(GLFWwindow* window, double xpos, double ypos) {
//     if (firstMouse) { lastX = xpos; lastY = ypos; firstMouse = false; }
//     if (leftButtonPressed && g_camera) {
//         // 这里可以实现类似你 CameraController.js 的旋转逻辑
//     }
//     lastX = xpos; lastY = ypos;
// }

// // 辅助函数：将 3D 坐标投影到 2D 屏幕
// glm::vec2 worldToScreen(glm::vec3 worldPos, glm::mat4 view, glm::mat4 proj, int screenW, int screenH) {
//     glm::vec4 clipPos = proj * view * glm::vec4(worldPos, 1.0f);
//     if (clipPos.w <= 0) return glm::vec2(-1, -1);
//     glm::vec3 ndc = glm::vec3(clipPos) / clipPos.w;
//     return glm::vec2(
//         (ndc.x + 1.0f) * 0.5f * screenW,
//         (1.0f - ndc.y) * 0.5f * screenH
//     );
// }

// // 辅助函数：根据鼠标位置计算 3D 射线
// glm::vec3 getRayFromMouse(float x, float y, int w, int h, glm::mat4 proj, glm::mat4 view) {
//     float normX = (2.0f * x) / w - 1.0f;
//     float normY = 1.0f - (2.0f * y) / h;
//     glm::vec4 rayClip = glm::vec4(normX, normY, -1.0, 1.0);
//     glm::vec4 rayEye = glm::inverse(proj) * rayClip;
//     rayEye = glm::vec4(rayEye.x, rayEye.y, -1.0, 0.0);
//     return glm::normalize(glm::vec3(glm::inverse(view) * rayEye));
// }

// Renderer::Renderer(int w, int h) : width(w), height(h) {
//     glfwInit();
//     glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
//     glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
//     glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
//     window = glfwCreateWindow(width, height, "AutoDrive Engine C++", NULL, NULL);
//     if (!window) { glfwTerminate(); }
    
//     glfwMakeContextCurrent(window);
//     gladLoadGLLoader((GLADloadproc)glfwGetProcAddress);

//     // --- 新增：初始化 ImGui ---
//     IMGUI_CHECKVERSION();
//     ImGui::CreateContext();
//     ImGui::StyleColorsDark();
//     ImGui_ImplGlfw_InitForOpenGL(window, true);
//     ImGui_ImplOpenGL3_Init("#version 430 core");

//     glEnable(GL_DEPTH_TEST);
//     glEnable(GL_PROGRAM_POINT_SIZE);
//     // --- 新增：开启混合 (Alpha Blending) ---
//     glEnable(GL_BLEND);
//     // 设置混合公式：结果 = (源颜色 * 源Alpha) + (目标颜色 * (1 - 源Alpha))
//     glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
//     // ------------------------------------
//     glfwSwapInterval(0); // 关闭垂直同步

//     // 初始化相机和图层
//     camera = std::make_unique<Camera>(glm::vec3(0, 30, 60));
//     g_camera = camera.get(); // 供回调使用
//     const int MAX_POINTS = 3300000;
//     pointCloud = std::make_unique<PointCloudLayer>(MAX_POINTS);
//     egoCarLayer = std::make_unique<BoxLayer>();
//     egoCarEdgeLayer = std::make_unique<EdgeBoxLayer>();
//     beltBatch = std::make_unique<BeltBatch>(200000);
//     obstacles = std::make_unique<ObstacleLayer>(300);
//     edges = std::make_unique<ObstacleEdgeLayer>(300);
//     sensorBackend = std::make_unique<MockSensorBackend>(MAX_POINTS);

//     // 3. 【关键：串联逻辑】
//     // 获取点云图层映射好的 GPU 内存指针
//     Point* gpuMemoryPtr = pointCloud->getMappedPointer();

//     // 把指针交给后端，并启动后台生成线程
//     if (gpuMemoryPtr) {
//         sensorBackend->start(gpuMemoryPtr);
//     }
// }

// Renderer::~Renderer() {
//     ImGui_ImplOpenGL3_Shutdown();
//     ImGui_ImplGlfw_Shutdown();
//     ImGui::DestroyContext();
//     glfwTerminate();
// }

// void Renderer::run() {
//     MockFrameGenerator generator;
//     int frameCount = 0;
//     double lastTime = glfwGetTime();
//     int nbFrames = 0;

//     // 确保这里关闭了垂直同步
//     glfwSwapInterval(0);
//     std::cout << "Step 1: Loop start" << std::endl;
//     while (!glfwWindowShouldClose(window)) {
//         float currentTime = (float)glfwGetTime();
//         nbFrames++;
        
//         // 如果距离上次更新超过 1 秒
//         if (currentTime - lastTime >= 1.0) {
//             double fps = double(nbFrames) / (currentTime - lastTime);
            
//             // 构造新的标题字符串
//             // 使用 (int)fps 让数字看起来更整洁，不跳动
//             std::string newTitle = "AutoDrive Engine C++ | FPS: " + std::to_string((int)fps);
            
//             // 更新窗口标题
//             glfwSetWindowTitle(window, newTitle.c_str());

//             // 重置计数器
//             nbFrames = 0;
//             lastTime = currentTime;
//         } 

//         MockFrame frame = generator.createFrame();
//         // std::cout << "Step 2: Matrix update" << std::endl;
//         // std::cout << "point cont" << frame.point_count << std::endl;

//         // if (frameCount % 60 == 0) {
//         //     std::cout << "\033[1;32m[Frame Info]\033[0m " 
//         //               << "Seq: " << frame.seq 
//         //               << " | EgoX: " << std::fixed << std::setprecision(2) << frame.ego_pos.x
//         //               << " | Vehicles: " << frame.polygons.size()
//         //               << " | Lines: " << frame.polylines.size() << std::endl;
//         // }
//         frameCount++;

//         // // 1. Update (数据处理)
//         // pointCloud->update(currentTime);

//         // 2. Render (绘制)
//         glClearColor(0.01f, 0.01f, 0.03f, 1.0f);
//         glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

//         // 2. 更新相机逻辑 (参数：位置, 偏航角, 跟随距离, 高度, 平滑系数)
//         camera->updateFollow(frame.ego_pos, 0.0f, 25.0f, 15.0f, 0.15f);
//         glm::mat4 view = camera->GetViewMatrix();
//         glm::mat4 proj = camera->GetProjectionMatrix((float)width, (float)height);

//         sensorBackend->setEgoPosition(frame.ego_pos);  
//         // pointCloud->render(view, proj);

//         // ---------------------------------------------------------
//         // 3. 【核心变更】渲染点云
//         // ---------------------------------------------------------
//         // 注意：不再调用 pointCloud->setLOD(...)，
//         // 我们直接把 frame.ego_pos 传进去，让 Compute Shader 执行 GPU 过滤。
//         pointCloud->render(view, proj, frame.ego_pos); 
//         // ---------------------------------------------------------

//         egoCarLayer->render(frame.ego_pos, 0.0f, glm::vec3(4.0f, 2.0f, 1.5f), view, proj);
//         egoCarEdgeLayer->render(frame.ego_pos, 0.0f, glm::vec3(4.0f, 2.0f, 1.5f), view, proj);

//         beltBatch->begin();
//         for (auto& line : frame.polylines) {
//             bool isDashed = (line.style.type == 3 || line.style.type == 4); // 简化逻辑
//             glm::vec3 color = glm::vec3(line.style.color); // MockFrameGenerator 已经转好了 vec4
            
//             // 参考 JS AMap.js 的双线逻辑
//             if (line.style.type == 2) { // 假设 2 是双线
//                 beltBatch->writeBelt(line.vertices, -0.3f, 0.5f, false, color);
//                 beltBatch->writeBelt(line.vertices,  0.3f, 0.5f, false, color);
//             } else {
//                 beltBatch->writeBelt(line.vertices, 0.0f, 0.5f, isDashed, color);
//             }
//         }
//         // 渲染
        
//         beltBatch->render(view, proj);

//         obstacles->updateData(frame.polygons); // 传入 Mock 数据
//         obstacles->render(view, proj);         // 绘制

//         edges->updateData(frame.polygons);
//         edges->render(view, proj);
//         glfwSwapBuffers(window);
//         glfwPollEvents();
//     }
// }

#include "renderer/Renderer.hpp"
#include "renderer/PointCloudLayer.hpp"
#include "data/MockFrameGenerator.hpp"
#include "renderer/BoxLayer.hpp"
#include "renderer/ObstacleLayer.hpp"
#include "renderer/ObstacleEdgeLayer.hpp"
#include <iostream>
#include <iomanip>
#include <set>      // 核心修复
#include <vector>   // 你的代码中用到了 vector
#include <string>   // 用于 std::to_string
#include <algorithm> // 用于 std::min, std::max 等
// --- ImGui 头文件 ---
#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"

// 辅助函数：3D 投影到 2D 屏幕
glm::vec2 worldToScreen(glm::vec3 worldPos, glm::mat4 view, glm::mat4 proj, int screenW, int screenH) {
    glm::vec4 clipPos = proj * view * glm::vec4(worldPos, 1.0f);
    if (clipPos.w <= 0) return glm::vec2(-1, -1);
    glm::vec3 ndc = glm::vec3(clipPos) / clipPos.w;
    return glm::vec2(
        (ndc.x + 1.0f) * 0.5f * (float)screenW,
        (1.0f - ndc.y) * 0.5f * (float)screenH
    );
}

// 辅助函数：鼠标像素转 3D 射线
glm::vec3 getRayFromMouse(float x, float y, int w, int h, glm::mat4 proj, glm::mat4 view) {
    float normX = (2.0f * x) / (float)w - 1.0f;
    float normY = 1.0f - (2.0f * y) / (float)h;
    glm::vec4 rayClip = glm::vec4(normX, normY, -1.0, 1.0);
    glm::vec4 rayEye = glm::inverse(proj) * rayClip;
    rayEye = glm::vec4(rayEye.x, rayEye.y, -1.0, 0.0);
    return glm::normalize(glm::vec3(glm::inverse(view) * rayEye));
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

    // 初始化 ImGui
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGui::StyleColorsDark();
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 430 core");

    glEnable(GL_DEPTH_TEST);
    glEnable(GL_PROGRAM_POINT_SIZE);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    
    glfwSwapInterval(0); 

    camera = std::make_unique<Camera>(glm::vec3(0, 30, 60));
    pointCloud = std::make_unique<PointCloudLayer>(3300000);
    egoCarLayer = std::make_unique<BoxLayer>();
    egoCarEdgeLayer = std::make_unique<EdgeBoxLayer>();
    beltBatch = std::make_unique<BeltBatch>(200000);
    obstacles = std::make_unique<ObstacleLayer>(300);
    edges = std::make_unique<ObstacleEdgeLayer>(300);
    sensorBackend = std::make_unique<MockSensorBackend>(3300000);

    Point* gpuMemoryPtr = pointCloud->getMappedPointer();
    if (gpuMemoryPtr) sensorBackend->start(gpuMemoryPtr);
}

Renderer::~Renderer() {
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
    glfwTerminate();
}

// void Renderer::run() {
//     MockFrameGenerator generator;
//     double lastTime = glfwGetTime();
//     int nbFrames = 0;
    
//     // --- 核心交互状态 ---
//     // int selectedIdx = -1;   single selection
//     // bool mouseReleased = true; // 用于防止长按连点

//     std::set<int> selectedIndices;  // multiple selections 
//     bool mouseReleased = true;

//     while (!glfwWindowShouldClose(window)) {
//         // 在 while 循环内部，MockFrame frame = generator.createFrame(); 之前插入
// // int fbWidth, fbHeight;
// // glfwGetFramebufferSize(window, &fbWidth, &fbHeight); // 物理像素（用于 glViewport）

// // int winWidth, winHeight;
// // glfwGetWindowSize(window, &winWidth, &winHeight);   // 逻辑坐标（用于射线计算）

// // // 更新 Renderer 的成员变量，确保其他地方引用的是最新值
// // this->width = winWidth;
// // this->height = winHeight;

// // // 关键：告诉 OpenGL 当前渲染区域的大小
// // glViewport(0, 0, fbWidth, fbHeight); 
//         int fbW, fbH, winW, winH;
//         glfwGetFramebufferSize(window, &fbW, &fbH);
//         glfwGetWindowSize(window, &winW, &winH);
//         this->width = winW; this->height = winH;
//         glViewport(0, 0, fbW, fbH); 
//         // --------------------------

//         // 1. ImGui 每一帧都要知道最新的窗口大小来布置 UI
//         ImGui_ImplOpenGL3_NewFrame();
//         ImGui_ImplGlfw_NewFrame();
//         ImGui::NewFrame();

//         float currentTime = (float)glfwGetTime();
//         nbFrames++;
        
//         if (currentTime - lastTime >= 1.0) {
//             double fps = double(nbFrames) / (currentTime - lastTime);
//             glfwSetWindowTitle(window, ("AutoDrive Engine C++ | FPS: " + std::to_string((int)fps)).c_str());
//             nbFrames = 0; lastTime = currentTime;
//         } 

//         // 1. ImGui 帧开始
//         ImGui_ImplOpenGL3_NewFrame();
//         ImGui_ImplGlfw_NewFrame();
//         ImGui::NewFrame();

//         MockFrame frame = generator.createFrame();


//         // 2. 交互逻辑
//         if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS && mouseReleased) {
//             double mx, my;
//             glfwGetCursorPos(window, &mx, &my);
            
//             // 使用【最新】的 width/height 计算
//             glm::mat4 view = camera->GetViewMatrix();
//             glm::mat4 proj = camera->GetProjectionMatrix((float)this->width, (float)this->height);
            
//             glm::vec3 rayDir = getRayFromMouse((float)mx, (float)my, this->width, this->height, proj, view);
//             // selectedIdx = obstacles->pickObstacle(camera->Position, rayDir);
            
//             mouseReleased = false;
//         }

//         // 2. 增强型交互逻辑：切换（Toggle）模式
//         int currentLeftState = glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT);
//         if (currentLeftState == GLFW_PRESS && mouseReleased) {
//             if (!ImGui::GetIO().WantCaptureMouse) {
//                 double mx, my;
//                 glfwGetCursorPos(window, &mx, &my);
                
//                 glm::mat4 view = camera->GetViewMatrix();
//                 glm::mat4 proj = camera->GetProjectionMatrix((float)width, (float)height);
//                 glm::vec3 rayDir = getRayFromMouse((float)mx, (float)my, width, height, proj, view);
//                 glm::vec3 rayOrigin = camera->Position; // 确认 Camera 成员名

//                 int hitIdx = obstacles->pickObstacle(rayOrigin, rayDir);

//                 if (hitIdx != -1) {
//                     // 如果点的是已经选中的，就取消选中；否则切换到新物体
//                     // if (selectedIdx == hitIdx) {
//                     //     selectedIdx = -1; 
//                     // } else {
//                     //     selectedIdx = hitIdx;
//                     // }
//                     if (selectedIndices.count(hitIdx)) {
//                         selectedIndices.erase(hitIdx); // 如果已经选中，则移除（取消选中）
//                     } else {
//                         selectedIndices.insert(hitIdx); // 如果未选中，则加入
//                     }
//                 } else {
//                     // 点击空白处取消选中
//                     // selectedIdx = -1;
//                     // 如果想点击空白处清空：selectedIndices.clear();
//                 }
//             }
//             mouseReleased = false; // 标记按键已按下，防止在一帧内触发多次切换
//         } else if (currentLeftState == GLFW_RELEASE) {
//             mouseReleased = true; // 只有松开后才允许下一次点击
//         }

//         // 3. 渲染原有的 3D 图层
//         glClearColor(0.01f, 0.01f, 0.03f, 1.0f);
//         glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

//         camera->updateFollow(frame.ego_pos, 0.0f, 25.0f, 15.0f, 0.15f);
//         glm::mat4 view = camera->GetViewMatrix();
//         glm::mat4 proj = camera->GetProjectionMatrix((float)width, (float)height);

//         sensorBackend->setEgoPosition(frame.ego_pos);  
//         pointCloud->render(view, proj, frame.ego_pos); 

//         egoCarLayer->render(frame.ego_pos, 0.0f, glm::vec3(4.0f, 2.0f, 1.5f), view, proj);
//         egoCarEdgeLayer->render(frame.ego_pos, 0.0f, glm::vec3(4.0f, 2.0f, 1.5f), view, proj);

//         // 渲染车道线
//         beltBatch->begin();
//         for (auto& line : frame.polylines) {
//             glm::vec3 color = glm::vec3(line.style.color);
//             beltBatch->writeBelt(line.vertices, 0.0f, 0.5f, (line.style.type >= 3), color);
//         }
//         beltBatch->render(view, proj);

//         // 渲染障碍物
//         obstacles->updateData(frame.polygons); 
//         obstacles->render(view, proj);         
//         edges->updateData(frame.polygons);
//         edges->render(view, proj);

//         // 4. 渲染 GUI (无关闭按钮，仅在选中时显示)
//         // if (selectedIdx != -1) {    single
//         //     const Polygon* obs = obstacles->getObstacle(selectedIdx);
//         //     if (obs) {
//         //         glm::vec2 screenPos = worldToScreen(obs->center, view, proj, width, height);
//         //         if (screenPos.x > 0) {
//         //             // 设置 UI 样式：稍微透明
//         //             ImGui::SetNextWindowBgAlpha(0.8f);
//         //             ImGui::SetNextWindowPos(ImVec2(screenPos.x + 15, screenPos.y - 15));
                    
//         //             // 注意：Begin 的第二个参数传 nullptr 就会移除右上角的 X 按钮
//         //             ImGui::Begin("InfoPanel", nullptr, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoMove);
                    
//         //             ImGui::TextColored(ImVec4(0.2f, 0.8f, 1.0f, 1.0f), "OBJECT: %s", obs->id.c_str());
//         //             ImGui::Separator();
//         //             ImGui::Text("Type: Vehicle");
//         //             ImGui::Text("Dist: %.1fm", glm::distance(frame.ego_pos, obs->center));
                    
//         //             ImGui::End();
//         //         }
//         //     }
//         // }

//         for (int idx : selectedIndices) {
//             const Polygon* obs = obstacles->getObstacle(idx);
//             if (obs) {
//                 glm::vec2 screenPos = worldToScreen(obs->center, view, proj, width, height);
//                 if (screenPos.x > 0) {
//                     ImGui::SetNextWindowBgAlpha(0.8f);
//                     ImGui::SetNextWindowPos(ImVec2(screenPos.x + 15, screenPos.y - 15));
                    
//                     // --- 关键：每个 ImGui 窗口必须有唯一的 ID ---
//                     // 使用 ID 字符串作为窗口标识，否则所有窗口会重叠合并
//                     std::string windowId = "Info##" + std::to_string(idx); 
                    
//                     ImGui::Begin(windowId.c_str(), nullptr, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoMove);
                    
//                     ImGui::TextColored(ImVec4(0.2f, 0.8f, 1.0f, 1.0f), "ID: %s", obs->id.c_str());
//                     ImGui::Separator();
//                     ImGui::Text("Dist: %.1fm", glm::distance(frame.ego_pos, obs->center));
                    
//                     ImGui::End();
//                 }
//             }
//         }

//         // 5. ImGui 渲染输出
//         ImGui::Render();
//         ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

//         glfwSwapBuffers(window);
//         glfwPollEvents();
//     }
// }


void Renderer::run() {
    MockFrameGenerator generator;
    double lastTime = glfwGetTime();
    int nbFrames = 0;
    
    // --- 核心交互状态 ---
    std::set<int> selectedIndices;  // 多选集合
    bool mouseReleased = true;      // 用于防止长按连点

    while (!glfwWindowShouldClose(window)) {
        // 1. 【同步窗口尺寸】 - 必须在所有逻辑之前
        int fbW, fbH, winW, winH;
        glfwGetFramebufferSize(window, &fbW, &fbH);
        glfwGetWindowSize(window, &winW, &winH);
        this->width = winW; 
        this->height = winH;
        glViewport(0, 0, fbW, fbH); 

        // 2. 【ImGui 准备新帧】 - 全局只需一次
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        // 计算 FPS
        float currentTime = (float)glfwGetTime();
        nbFrames++;
        if (currentTime - lastTime >= 1.0) {
            double fps = double(nbFrames) / (currentTime - lastTime);
            glfwSetWindowTitle(window, ("AutoDrive Engine C++ | FPS: " + std::to_string((int)fps)).c_str());
            nbFrames = 0; lastTime = currentTime;
        } 

        // 获取当前帧数据
        MockFrame frame = generator.createFrame();

        // 3. 【修正后的交互逻辑】
        int currentLeftState = glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT);
        if (currentLeftState == GLFW_PRESS) {
            if (mouseReleased) {
                // 只有当鼠标点击不在 UI 按钮上时，才处理 3D 拾取
                if (!ImGui::GetIO().WantCaptureMouse) {
                    double mx, my;
                    glfwGetCursorPos(window, &mx, &my);
                    
                    glm::mat4 view = camera->GetViewMatrix();
                    glm::mat4 proj = camera->GetProjectionMatrix((float)this->width, (float)this->height);
                    
                    glm::vec3 rayDir = getRayFromMouse((float)mx, (float)my, this->width, this->height, proj, view);
                    glm::vec3 rayOrigin = camera->Position;

                    int hitIdx = obstacles->pickObstacle(rayOrigin, rayDir);

                    if (hitIdx != -1) {
                        // 切换选择状态
                        if (selectedIndices.count(hitIdx)) {
                            selectedIndices.erase(hitIdx); 
                        } else {
                            selectedIndices.insert(hitIdx); 
                        }
                    } else {
                        // 如果点击的是空白处，清空所有选择
                        selectedIndices.clear();
                    }
                }
                mouseReleased = false; // 锁定，直到松开鼠标
            }
        } else {
            mouseReleased = true; // 松开鼠标，解锁
        }

        // 4. 【渲染 3D 场景】
        glClearColor(0.01f, 0.01f, 0.03f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        camera->updateFollow(frame.ego_pos, 0.0f, 25.0f, 15.0f, 0.15f);
        glm::mat4 view = camera->GetViewMatrix();
        glm::mat4 proj = camera->GetProjectionMatrix((float)this->width, (float)this->height);

        sensorBackend->setEgoPosition(frame.ego_pos);  
        pointCloud->render(view, proj, frame.ego_pos); 

        egoCarLayer->render(frame.ego_pos, 0.0f, glm::vec3(4.0f, 2.0f, 1.5f), view, proj);
        egoCarEdgeLayer->render(frame.ego_pos, 0.0f, glm::vec3(4.0f, 2.0f, 1.5f), view, proj);

        beltBatch->begin();
        for (auto& line : frame.polylines) {
            glm::vec3 color = glm::vec3(line.style.color);
            beltBatch->writeBelt(line.vertices, 0.0f, 0.5f, (line.style.type >= 3), color);
        }
        beltBatch->render(view, proj);

        obstacles->updateData(frame.polygons); 
        obstacles->render(view, proj);         
        edges->updateData(frame.polygons);
        edges->render(view, proj);

        // 5. 【渲染多个 Label UI】
        for (int idx : selectedIndices) {
            const Polygon* obs = obstacles->getObstacle(idx);
            if (obs) {
                glm::vec2 screenPos = worldToScreen(obs->center, view, proj, this->width, this->height);
                // 确保物体在屏幕范围内（裁剪掉相机背后的物体）
                if (screenPos.x > 0 && screenPos.x < this->width && screenPos.y > 0 && screenPos.y < this->height) {
                    ImGui::SetNextWindowBgAlpha(0.8f);
                    ImGui::SetNextWindowPos(ImVec2(screenPos.x + 15, screenPos.y - 15));
                    
                    // 使用唯一标识符 ##idx
                    std::string windowId = "Info##" + std::to_string(idx); 
                    
                    ImGui::Begin(windowId.c_str(), nullptr, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoMove);
                    ImGui::TextColored(ImVec4(0.2f, 0.8f, 1.0f, 1.0f), "ID: %s", obs->id.c_str());
                    ImGui::Separator();
                    ImGui::Text("Dist: %.1fm", glm::distance(frame.ego_pos, obs->center));
                    ImGui::End();
                }
            }
        }

        // 6. 【最终绘制 UI】
        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        glfwSwapBuffers(window);
        glfwPollEvents();
    }
}