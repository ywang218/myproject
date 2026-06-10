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

    // ★【手搓 三缓冲】之四：创建并持久映射 NUM_BUFFERS 块输入缓冲，全部入 freeList
    // 2. 初始化三块 Input SSBO (Persistent Mapping)，全部加入空闲列表
    GLbitfield flags = GL_MAP_WRITE_BIT | GL_MAP_PERSISTENT_BIT | GL_MAP_COHERENT_BIT;
    glGenBuffers(NUM_BUFFERS, inputSSBO);
    for (int i = 0; i < NUM_BUFFERS; ++i) {
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, inputSSBO[i]);
        glBufferStorage(GL_SHADER_STORAGE_BUFFER, pointCount * sizeof(Point), nullptr, flags);
        mappedPtr[i] = (Point*)glMapBufferRange(GL_SHADER_STORAGE_BUFFER, 0, pointCount * sizeof(Point), flags);
        freeList.push_back(i);
    }

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

    // 性能探针：GPU 计时 query 对象 (compute / draw 各一个)
    glGenQueries(1, &queryCompute);
    glGenQueries(1, &queryDraw);

    uEgoPosLoc = glGetUniformLocation(filterShader->ID, "uEgoPos");
    uTotalPointsLoc = glGetUniformLocation(filterShader->ID, "uTotalPoints");


    // 4. 修改原子计数器和间接指令 Buffer
    // 我们将它们合二为一，或者确保 atomicBuffer 的布局符合 IndirectCommand 结构
    glGenBuffers(1, &indirectBuffer);
    glBindBuffer(GL_DRAW_INDIRECT_BUFFER, indirectBuffer);
    DrawArraysIndirectCommand cmd = { 0, 1, 0, 0 };
    glBufferData(GL_DRAW_INDIRECT_BUFFER, sizeof(DrawArraysIndirectCommand), &cmd, GL_DYNAMIC_DRAW);
}

PointCloudLayer::~PointCloudLayer() {
    for (int i = 0; i < NUM_BUFFERS; ++i) {
        if (fences[i]) glDeleteSync(fences[i]);
        if (mappedPtr[i]) {
            glBindBuffer(GL_SHADER_STORAGE_BUFFER, inputSSBO[i]);
            glUnmapBuffer(GL_SHADER_STORAGE_BUFFER);
        }
    }
    glDeleteBuffers(NUM_BUFFERS, inputSSBO);
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


// ╔════════════════════════════════════════════════════════════════════════╗
// ║ 【手搓 三缓冲 + FENCE】之二：生产者/消费者交接 (acquire/publish/consume) ║
// ║ worker 线程: acquireWriteBuffer() 取空闲块 → 写 → publishFrame() 发布     ║
// ║ 渲染线程: consumeAndFilter() 回收 fence 已完成的块 + 消费最新帧去剔除      ║
// ║ freeList/readyList 两个队列 + poolMtx 实现跨线程交接, 全程仅渲染线程碰 GL ║
// ╚════════════════════════════════════════════════════════════════════════╝

// [worker 线程, 无 GL] 取一块空闲输入缓冲。无空闲则返回 nullptr (本帧丢弃)。
Point* PointCloudLayer::acquireWriteBuffer(int& outIndex) {
    std::lock_guard<std::mutex> lock(poolMtx);
    if (freeList.empty()) return nullptr;
    outIndex = freeList.back();
    freeList.pop_back();
    return mappedPtr[outIndex];
}

// [worker 线程] 写完发布该帧 (索引 / egoPos / 实际点数)。
void PointCloudLayer::publishFrame(int index, const glm::vec3& ego, uint32_t count) {
    std::lock_guard<std::mutex> lock(poolMtx);
    readyList.push_back({index, ego, (count > (uint32_t)pointCount) ? (uint32_t)pointCount : count});
}

// [渲染线程] 回收 fence 已完成的缓冲，消费最新就绪帧并剔除；剔除了返回 true。
bool PointCloudLayer::consumeAndFilter() {
    // 1. 回收：fence 已 signaled 的输入缓冲归还空闲列表
    for (int i = 0; i < NUM_BUFFERS; ++i) {
        if (fences[i]) {
            GLenum r = glClientWaitSync(fences[i], 0, 0); // 非阻塞轮询
            if (r == GL_ALREADY_SIGNALED || r == GL_CONDITION_SATISFIED) {
                glDeleteSync(fences[i]);
                fences[i] = nullptr;
                std::lock_guard<std::mutex> lock(poolMtx);
                freeList.push_back(i);
            }
        }
    }

    // 2. 取最新就绪帧；被跳过的旧帧 (未被 GPU 读过、无 fence) 直接归还空闲列表
    ReadyFrame frame;
    {
        std::lock_guard<std::mutex> lock(poolMtx);
        if (readyList.empty()) return false;
        while (readyList.size() > 1) {
            freeList.push_back(readyList.front().index);
            readyList.pop_front();
        }
        frame = readyList.front();
        readyList.pop_front();
    }

    // 3. 剔除
    runFilter(frame.index, frame.ego, frame.count);
    return true;
}

// [渲染线程内部] 绑定 inputSSBO[index]、按 count 设 uTotalPoints、dispatch、屏障，并打 fence。
void PointCloudLayer::runFilter(int index, const glm::vec3& egoPos, uint32_t count) {
    filterRuns++;
    lastInputCount = count;

    // 重置间接绘制指令 (vertexCount 清零, instanceCount=1)
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, indirectBuffer);
    uint32_t resetData[4] = { 0, 1, 0, 0 };
    glBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, sizeof(resetData), resetData);

    filterShader->use();
    // 点是传感器相对坐标，按「距传感器原点」的距离剔除 (LiDAR 量程本就是相对自身)
    (void)egoPos;
    glm::vec3 sensorOrigin(0.0f);
    glUniform3fv(uEgoPosLoc, 1, &sensorOrigin[0]);
    glUniform1ui(uTotalPointsLoc, (GLuint)count); // 真实/本帧实际点数，而非固定满值

    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, inputSSBO[index]);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, outputSSBO);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, indirectBuffer);

    // GPU 计时：先取回上一次结果 (已 available 才取)，再发起本次测量
    if (qcPending) {
        GLint avail = 0;
        glGetQueryObjectiv(queryCompute, GL_QUERY_RESULT_AVAILABLE, &avail);
        if (avail) {
            GLuint64 ns = 0;
            glGetQueryObjectui64v(queryCompute, GL_QUERY_RESULT, &ns);
            lastComputeMs = (double)ns / 1.0e6;
            qcPending = false;
        }
    }
    bool measure = !qcPending;
    if (measure) glBeginQuery(GL_TIME_ELAPSED, queryCompute);
    glDispatchCompute((count + 255) / 256, 1, 1);
    if (measure) { glEndQuery(GL_TIME_ELAPSED); qcPending = true; }

    glMemoryBarrier(GL_COMMAND_BARRIER_BIT | GL_SHADER_STORAGE_BARRIER_BIT | GL_VERTEX_ATTRIB_ARRAY_BARRIER_BIT);

    // ★【手搓 FENCE】之三：剔除后给该输入缓冲打 fence。
    // GPU 真正读完此缓冲 (含把保留点写进 outputSSBO) 后 fence 才 signaled，
    // consumeAndFilter() 轮询到它 signaled 才把缓冲放回 freeList → worker 不会覆写在读的数据。
    if (fences[index]) glDeleteSync(fences[index]);
    fences[index] = glFenceSync(GL_SYNC_GPU_COMMANDS_COMPLETE, 0);
}

// 每帧调用：用上一次剔除的结果 (outputSSBO + indirectBuffer) 做间接绘制。
// egoWorld：本帧实时车辆位置；着色器据此把传感器相对坐标摆进世界 → 点云平滑跟车。
void PointCloudLayer::draw(const glm::mat4& view, const glm::mat4& projection, const glm::vec3& egoWorld) {
    renderShader->use();
    renderShader->setMat4("uView", view);
    renderShader->setMat4("uProjection", projection);
    renderShader->setVec3("uEgoWorld", egoWorld);

    glBindVertexArray(vao);
    glBindBuffer(GL_ARRAY_BUFFER, outputSSBO);

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, sizeof(Point), (void*)offsetof(Point, pos));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, sizeof(Point), (void*)offsetof(Point, color));

    glBindBuffer(GL_DRAW_INDIRECT_BUFFER, indirectBuffer);

    if (qdPending) {
        GLint avail = 0;
        glGetQueryObjectiv(queryDraw, GL_QUERY_RESULT_AVAILABLE, &avail);
        if (avail) {
            GLuint64 ns = 0;
            glGetQueryObjectui64v(queryDraw, GL_QUERY_RESULT, &ns);
            lastDrawMs = (double)ns / 1.0e6;
            qdPending = false;
        }
    }
    bool measure = !qdPending;
    if (measure) glBeginQuery(GL_TIME_ELAPSED, queryDraw);
    glDrawArraysIndirect(GL_POINTS, (void*)0);
    if (measure) { glEndQuery(GL_TIME_ELAPSED); qdPending = true; }

    glBindBuffer(GL_DRAW_INDIRECT_BUFFER, 0);
    glBindVertexArray(0);
}

// 回读 indirect buffer 里 compute 写入的实际绘制点数。
// 会触发 GPU->CPU 同步，仅用于诊断，建议每秒调用一次。
uint32_t PointCloudLayer::readDrawnCount() {
    uint32_t count = 0;
    glBindBuffer(GL_DRAW_INDIRECT_BUFFER, indirectBuffer);
    glGetBufferSubData(GL_DRAW_INDIRECT_BUFFER, 0, sizeof(uint32_t), &count);
    glBindBuffer(GL_DRAW_INDIRECT_BUFFER, 0);
    return count;
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