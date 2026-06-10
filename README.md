# AutoDrive Engine (C++ / OpenGL)

一个 C++ / OpenGL 4.3 的自动驾驶场景实时渲染器：模拟激光雷达点云、障碍物、车道线与自车，
采用 GPU-driven 管线（compute 剔除 + indirect draw + persistent mapping + 三缓冲/fence）。

> 📖 渲染架构与优化设计详见 **[RENDERING_OPTIMIZATION.md](./RENDERING_OPTIMIZATION.md)**。

---

## 功能特性

- **点云**：后台 worker 线程 ~10Hz 生成，GPU compute shader 按距离做 LOD 抽稀，indirect 绘制。
- **三缓冲 + Fence**：worker 写 / GPU 读 零拷贝、无撕裂的生产者-消费者交接。
- **障碍物**：实例化渲染（填充 + 边框），支持鼠标射线拾取与 ImGui 信息标签。
- **车道线**：条带（belt）批量渲染。
- **相机**：平滑跟随自车。
- **性能探针**：标题栏/控制台每秒输出 FPS、compute/draw 的 GPU 耗时、剔除频率、输入/绘制点数。

---

## 构建

**依赖**：C++17 编译器、CMake ≥ 3.10、OpenGL、GLFW3、TBB、pthread。
（GLAD / GLM / Dear ImGui 已随仓库自带于 `external/`。）

Ubuntu / WSL 安装系统依赖：

```bash
sudo apt install build-essential cmake libglfw3-dev libgl-dev libtbb-dev
```

配置并编译：

```bash
cmake -S . -B build
cmake --build build -j
```

> 默认 `Release` + `-O3 -march=native -ffast-math`（见 `CMakeLists.txt`）。

---

## 运行

从**项目根目录**运行（着色器以工作目录下的 `shaders/` 为相对路径加载）：

```bash
./build/app
```

启动时会打印 OpenGL 渲染器信息，例如：

```
[GL] Vendor  : ...
[GL] Renderer: ...        # 若为 llvmpipe/softpipe 等则是软件渲染，无 GPU 加速
[GL] Version : ...
```

> ⚠️ 若 `Renderer` 是软件渲染器（如 WSL 未开 GPU 直通），帧率会受 CPU 光栅化限制，
> **不反映代码的真实性能**。详见优化文档第 5 节。

---

## 操作

| 输入 | 行为 |
|---|---|
| 鼠标左键点击障碍物 | 选中/取消选中（切换），显示 ImGui 信息标签 |
| 点击空白处 | 清空选择 |
| 相机 | 自动平滑跟随自车 |

---

## 目录结构

```
include/renderer/   渲染层头文件 (PointCloudLayer / MockSensorBackend / ObstacleLayer ...)
include/data/       数据结构 (MockFrameGenerator)
src/renderer/       渲染实现 (Renderer / PointCloudLayer)
src/data/           数据生成实现
shaders/            GLSL (point_cloud.vert/frag, filter.comp, obstacle.*, belt.* ...)
external/           GLAD / GLM / Dear ImGui
RENDERING_OPTIMIZATION.md   架构与优化设计文档
```

---

## 已知待办

- 仿真推进按帧步进 → 速度绑定帧率，应改固定时间步。
- 改 shader 后需重新 `cmake -S . -B build`（或手动 `cp shaders/* build/shaders/`），
  因为 `file(COPY ...)` 只在 configure 时执行。
- 收尾项 c：`COHERENT` 映射 → `MAP_FLUSH_EXPLICIT_BIT` + `glFlushMappedBufferRange`。
