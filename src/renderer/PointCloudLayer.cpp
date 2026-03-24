#include "renderer/PointCloudLayer.hpp"
#include <glm/gtc/type_ptr.hpp>
#include <iostream>

PointCloudLayer::PointCloudLayer(int count) : pointCount(count) {
    // 1. 加载着色器
    renderShader = std::make_unique<Shader>("shaders/point_cloud.vert", "shaders/point_cloud.frag");
    // 加载 Compute Shader (假设你已经写好了 filter.comp)
    filterShader = std::make_unique<Shader>("shaders/filter.comp"); 

    if (renderShader->ID == 0 || filterShader->ID == 0) {
        std::cerr << "CRITICAL ERROR: Shader compilation failed!" << std::endl;
        exit(-1); // 尽早退出，防止后面段错误
    }

    // 2. 初始化 Input SSBO (Persistent Mapping)
    glGenBuffers(1, &inputSSBO);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, inputSSBO);
    GLbitfield flags = GL_MAP_WRITE_BIT | GL_MAP_PERSISTENT_BIT | GL_MAP_COHERENT_BIT;
    glBufferStorage(GL_SHADER_STORAGE_BUFFER, pointCount * sizeof(Point), nullptr, flags);
    sharedMappedPtr = (Point*)glMapBufferRange(GL_SHADER_STORAGE_BUFFER, 0, pointCount * sizeof(Point), flags);

    // 3. 初始化 Output SSBO (GPU内部写入用)
    glGenBuffers(1, &outputSSBO);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, outputSSBO);
    // 这里不需要 Map，直接分配显存空间即可
    glBufferData(GL_SHADER_STORAGE_BUFFER, pointCount * sizeof(Point), nullptr, GL_STREAM_DRAW);

    // 4. 初始化 Atomic Counter Buffer (用于存储点数计数)
    glGenBuffers(1, &atomicBuffer);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, atomicBuffer);
    uint32_t zero = 0;
    glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(uint32_t), &zero, GL_DYNAMIC_COPY);

    // 5. 初始化 VAO (空VAO即可，因为我们直接从 SSBO 读数据)
    glGenVertexArrays(1, &vao);

    uEgoPosLoc = glGetUniformLocation(filterShader->ID, "uEgoPos");
    uTotalPointsLoc = glGetUniformLocation(filterShader->ID, "uTotalPoints");
}

PointCloudLayer::~PointCloudLayer() {
    if (sharedMappedPtr) {
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, inputSSBO);
        glUnmapBuffer(GL_SHADER_STORAGE_BUFFER);
    }
    glDeleteBuffers(1, &inputSSBO);
    glDeleteBuffers(1, &outputSSBO);
    glDeleteBuffers(1, &atomicBuffer);
    glDeleteVertexArrays(1, &vao);
}

// void PointCloudLayer::render(const glm::mat4& view, const glm::mat4& projection, const glm::vec3& egoPos) {
//     if (!sharedMappedPtr) return;

//     // --- 第一阶段：Compute Shader 过滤 ---
//     filterShader->use();
    
//     // 重置原子计数器
//     uint32_t zero = 0;
//     glBindBuffer(GL_SHADER_STORAGE_BUFFER, atomicBuffer);
//     glBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, sizeof(uint32_t), &zero);

//     // 绑定 Buffer 插槽 (需与 filter.comp 中的 binding 一致)
//     glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, inputSSBO);
//     glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, outputSSBO);
//     glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, atomicBuffer);

//     filterShader->setVec3("uEgoPos", egoPos);
//     // filterShader->setInt("uTotalPoints", pointCount);
//     glUniform1ui(glGetUniformLocation(filterShader->ID, "uTotalPoints"), (GLuint)pointCount);

//     // 启动计算：每个工作组 256 个线程
//     glDispatchCompute((pointCount + 255) / 256, 1, 1);

//     // 【关键】同步屏障：确保写入 outputSSBO 完成后，后续渲染才能读取
//     glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT 
//     // | GL_BUFFER_UPDATE_BARRIER_BIT
//     );

//     // 6. 回读测试
//     uint32_t activePoints2 = 0;
//     glBindBuffer(GL_SHADER_STORAGE_BUFFER, atomicBuffer);
//     glGetBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, sizeof(uint32_t), &activePoints2);

//     std::cout << "[GPU Debug] Active Points: " << activePoints2 << std::endl;

//     // --- 第二阶段：顶点渲染 ---
//     renderShader->use();
//     renderShader->setMat4("uView", view);
//     renderShader->setMat4("uProjection", projection);

//     // 从 Atomic Buffer 读回过滤后的实际点数 (回读 CPU)
//     uint32_t activePoints = 0;
//     glBindBuffer(GL_SHADER_STORAGE_BUFFER, atomicBuffer);
//     glGetBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, sizeof(uint32_t), &activePoints);
//     std::cout << "[GPU Debug] Active Points: " << activePoints << " / " << pointCount << std::endl;
    
//     Point firstPoint;
//     glBindBuffer(GL_SHADER_STORAGE_BUFFER, inputSSBO);
//     glGetBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, sizeof(Point), &firstPoint);
//     std::cout << "Input Point 0: " << firstPoint.pos.x << ", " << firstPoint.pos.y << std::endl;

//     // 绑定数据源为 OutputSSBO 进行绘制
//     glBindVertexArray(vao);
//     glBindBuffer(GL_ARRAY_BUFFER, outputSSBO);
    
//     // 设置顶点属性指向 OutputSSBO 中的数据
//     glEnableVertexAttribArray(0); // pos
//     glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, sizeof(Point), (void*)offsetof(Point, pos));
//     glEnableVertexAttribArray(1); // color
//     glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, sizeof(Point), (void*)offsetof(Point, color));

//     // 只画过滤后的有效点
//     glDrawArrays(GL_POINTS, 0, activePoints);
    
//     // 解绑
//     glDisableVertexAttribArray(0);
//     glDisableVertexAttribArray(1);
//     glBindBuffer(GL_ARRAY_BUFFER, 0);
// }


void PointCloudLayer::render(const glm::mat4& view, const glm::mat4& projection, const glm::vec3& egoPos) {
    if (!sharedMappedPtr || !filterShader || !renderShader) return;

    // --- 第一阶段：同步 ---
    // 确保后台线程通过 Persistent Mapping 写入的数据对 GPU 可见
    glMemoryBarrier(GL_CLIENT_MAPPED_BUFFER_BARRIER_BIT);
     

    // 1. 重置计数器
    uint32_t zero = 0;
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, atomicBuffer);
    glBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, sizeof(uint32_t), &zero);
    glMemoryBarrier(GL_BUFFER_UPDATE_BARRIER_BIT); // 确保重置完成


    // std::cout << "DEBUG: Shader Use" << std::endl;
    // 2. 运行 Compute Shader
    filterShader->use();
    
    // 安全地获取 Location
    // GLint locEgo = glGetUniformLocation(filterShader->ID, "uEgoPos");
    // GLint locTotal = glGetUniformLocation(filterShader->ID, "uTotalPoints");

    // if (locEgo != -1) 
    glUniform3fv(uEgoPosLoc, 1, &egoPos[0]);
    // if (locTotal != -1)
     glUniform1ui(
        //locTotal
        uTotalPointsLoc
        , (GLuint)pointCount);

    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, inputSSBO);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, outputSSBO);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, atomicBuffer);

    // 每一组 256 线程
    GLuint numGroups = (pointCount + 255) / 256;

    // std::cout << "DEBUG: Dispatch" << std::endl;
    glDispatchCompute(numGroups, 1, 1);

    // 3. 强力同步
    glMemoryBarrier(GL_ALL_BARRIER_BITS | GL_ATOMIC_COUNTER_BARRIER_BIT);
    // glFinish(); 

    glFlush();

    // std::cout << "DEBUG: Readback" << std::endl;
    // 4. 读取计数器
    uint32_t activePoints = 0;
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, atomicBuffer);
    glGetBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, sizeof(uint32_t), &activePoints);

    // 强制限制，防止 glDrawArrays 崩溃
    activePoints = std::min(activePoints, (uint32_t)pointCount);
    
    if (activePoints > 0) {
        // std::cout << "DEBUG: Draw: " << activePoints << std::endl;
        renderShader->use();
        renderShader->setMat4("uView", view);
        renderShader->setMat4("uProjection", projection);

        glBindVertexArray(vao);
        glBindBuffer(GL_ARRAY_BUFFER, outputSSBO);
        
        glEnableVertexAttribArray(0); 
        glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, sizeof(Point), (void*)offsetof(Point, pos));
        glEnableVertexAttribArray(1); 
        glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, sizeof(Point), (void*)offsetof(Point, color));

        glDrawArrays(GL_POINTS, 0, (GLsizei)activePoints);
        
        glBindVertexArray(0);
    }
}




// #include "renderer/PointCloudLayer.hpp"
// #include <glm/gtc/type_ptr.hpp>

// PointCloudLayer::PointCloudLayer(int count) : pointCount(count) {
//     // 1. 加载 Shader
//     // 注意：暂时去掉了 computeShader，因为我们现在改用 CPU 后端模拟。
//     // 如果以后要用 GPU 生成，再加回来。
//     renderShader = std::make_unique<Shader>("shaders/point_cloud.vert", "shaders/point_cloud.frag");

//     // 2. 初始化 Persistent Mapping SSBO
//     glGenBuffers(1, &ssbo);
//     glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssbo);
    
//     // 设置标志位：
//     // GL_MAP_WRITE_BIT: 我们要写它
//     // GL_MAP_PERSISTENT_BIT: 映射后指针永久有效，不用每帧 map/unmap
//     // GL_MAP_COHERENT_BIT: 写入后立即对 GPU 可见，无需手动执行 flush
//     GLbitfield flags = GL_MAP_WRITE_BIT | GL_MAP_PERSISTENT_BIT | GL_MAP_COHERENT_BIT;
    
//     // 创建不可变存储 (Immutable Storage)
//     glBufferStorage(GL_SHADER_STORAGE_BUFFER, pointCount * sizeof(Point), nullptr, flags);
    
//     // 获取映射指针
//     sharedMappedPtr = (Point*)glMapBufferRange(GL_SHADER_STORAGE_BUFFER, 0, 
//                                                pointCount * sizeof(Point), flags);
    
//     // 绑定到插槽 0 (Shader 里的 binding = 0 对应这里)
//     glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, ssbo);

//     // 3. 初始化 VAO
//     glGenVertexArrays(1, &vao);
// }

// PointCloudLayer::~PointCloudLayer() {
//     if (sharedMappedPtr) {
//         glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssbo);
//         glUnmapBuffer(GL_SHADER_STORAGE_BUFFER);
//     }
//     glDeleteBuffers(1, &ssbo);
//     glDeleteVertexArrays(1, &vao);
// }

// void PointCloudLayer::render(const glm::mat4& view, const glm::mat4& projection) {
//     if (!sharedMappedPtr) return;

//     renderShader->use();
//     renderShader->setMat4("uView", view);
//     renderShader->setMat4("uProjection", projection);

//     // 点云绘制只需要绑定 VAO
//     // 数据已经在渲染之前由 Backend 线程静默写入 SSBO 了
//     glBindVertexArray(vao);
//     int drawCount = pointCount / currentStride;
//     // std::cout << "drawcount: " << drawCount << "current stride: " << currentStride << std::endl;
//     glDrawArrays(GL_POINTS, 0, drawCount);
// }