# Compute Shader 三端对比：OpenGL(C++) · WebGPU(WGSL) · Three.js(WebGPURenderer)

> 围绕同一个任务：**用 Compute Shader 对点云做距离剔除（GPU 端流压缩 stream compaction），再用 indirect draw 把保留下来的点画出来**。
> 三套实现做对照，方便对照学习从 OpenGL 迁移到 Web 的思路。

---

## 目录

1. [filter.comp 与 WGSL 的相似与区别（绑定点 0/1/2 详解）](#1-filtercomp-与-wgsl-的相似与区别)
2. [实现一：C++ / OpenGL 4.3 完整实现](#2-实现一c--opengl-43-完整实现)
3. [实现二：React + WGSL（裸 WebGPU）完整实现](#3-实现二react--wgsl裸-webgpu完整实现)
4. [实现三：React + Three.js WebGPURenderer 完整实现](#4-实现三react--threejs-webgpurenderer-完整实现)
5. [三端横向对比表](#5-三端横向对比表)

---

## 1. filter.comp 与 WGSL 的相似与区别

你这个 `filter.comp` 是 **OpenGL 4.3 的 GLSL Compute Shader**（不是 Vulkan GLSL，也不是 WGSL）。下面对照着 WebGPU/WGSL 逐点拆解。

### 1.1 整体对应关系

| 概念 | GLSL (OpenGL) | WebGPU / WGSL |
|---|---|---|
| 着色器语言 | GLSL 430 | WGSL |
| 工作组大小 | `layout(local_size_x=256) in;` | `@workgroup_size(256)` |
| 存储缓冲 | SSBO `buffer` | `var<storage>` |
| 绑定声明 | `layout(binding=N)` | `@group(G) @binding(N)` |
| 绑定数据 | `glBindBufferBase(target, N, buf)` | `BindGroup` + `setBindGroup` |
| 全局索引 | `gl_GlobalInvocationID.x` | `@builtin(global_invocation_id)` |
| 组内索引 | `gl_LocalInvocationID.x` | `@builtin(local_invocation_id)` |
| 同步 | `barrier()` | `workgroupBarrier()` |
| 共享内存 | `shared` | `var<workgroup>` |
| 原子操作 | `atomicAdd(x, v)` | `atomicAdd(&x, v)` |
| 派发 | `glDispatchCompute(gx,gy,gz)` | `dispatchWorkgroups(gx,gy,gz)` |

### 1.2 绑定点（binding 0/1/2）—— 核心区别

**你的 GLSL 写法：**

```glsl
layout(std430, binding = 0) readonly  buffer InBuf      { Point inPoints[]; };
layout(std430, binding = 1) writeonly buffer OutBuf     { Point outPoints[]; };
layout(std430, binding = 2)           buffer IndirectBuf { uint vertexCount; ... } indirect;
```

C++ 侧：

```cpp
glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, inputSSBO[index]);
glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, outputSSBO);
glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, indirectBuffer);
```

**OpenGL 的绑定模型是「扁平 + 全局」的**：只有一个数字 `binding`，直接对应一个全局的「SSBO 绑定点表」槽位。Shader 里 `binding=0`，C++ 里就 `glBindBufferBase(..., 0, ...)`，靠这个 0 号槽把两边对上。没有「组」的概念。

**WGSL 等价写法：**

```wgsl
struct Point {
    pos: vec4<f32>,
    color: vec4<f32>,
};

struct Indirect {
    vertexCount: atomic<u32>,   // 要原子访问就必须声明成 atomic
    instanceCount: u32,
    firstVertex: u32,
    baseInstance: u32,
};

@group(0) @binding(0) var<storage, read>       inPoints  : array<Point>;
@group(0) @binding(1) var<storage, read_write> outPoints : array<Point>;
@group(0) @binding(2) var<storage, read_write> indirect  : Indirect;
```

**WGSL 的绑定模型是「两级」的：`@group(G) @binding(N)`。** 多了一个 `group` 维度，这是它和 GLSL 最本质的区别。

### 1.3 为什么 WebGPU 要多一层 group？

OpenGL 是「每次 draw/dispatch 前一个一个 bind」，每帧重新绑定，开销分散且隐式。WebGPU 把绑定**打包成不可变的 `BindGroup` 对象**，按更新频率分组：

- `@group(0)` 放每帧不变的（相机、全局参数）
- `@group(1)` 放每个物体变的（模型矩阵、这批点云的 buffer）

切换物体时只需换 `group(1)`，`group(0)` 保持不动，驱动层校验一次后整组复用，CPU 开销远小于 OpenGL 一个个 `glBindBufferBase`。

```
GLSL:  binding=0  ←─ glBindBufferBase(target, 0, buf)                       // 一级，全局槽
WGSL:  @group(0) @binding(0)  ←─ setBindGroup(0, group{entries:[{binding:0}]})  // 二级
```

### 1.4 几个容易踩坑的具体差异

1. **`readonly`/`writeonly` → `read`/`read_write`**：WGSL 没有单独的 `write` 模式，要写就是 `read_write`。
2. **原子访问必须声明 `atomic<T>`**：GLSL 可对普通 `uint` 做 `atomicAdd`；WGSL 中被原子访问的字段必须类型层面就是 `atomic<u32>`，且只能用 `atomicAdd(&x, v)`（传指针），普通读写要用 `atomicLoad/atomicStore`。
3. **`barrier()` → `workgroupBarrier()`**：WGSL 还细分 `workgroupBarrier()`（共享内存）与 `storageBarrier()`（storage buffer）。
4. **内存布局**：GLSL 用 `std430`；WGSL 没有显式 layout，规则接近但不完全等于 std430（`vec3<f32>` 对齐到 16 字节）。本例全是 `vec4`，两边都 16 字节对齐，不会出问题；但若改成 `vec3 pos`，偏移可能对不上。
5. **间接绘制**：`glDrawArraysIndirect` → `pass.drawIndirect(buffer, offset)`，且 buffer 创建时必须带 `GPUBufferUsage.INDIRECT`。

---

## 2. 实现一：C++ / OpenGL 4.3 完整实现

这是你项目里**实际运行的代码**。算法：compute 过滤 → 原子压缩写出 → indirect 绘制；外加手搓三缓冲 + fence 做 CPU/GPU 异步交接。

### 2.1 Compute Shader：`shaders/filter.comp`

```glsl
#version 430 core
layout (local_size_x = 256) in;

struct Point {
    vec4 pos;
    vec4 color;
};

// 绑定点 0: 原始全量数据
layout(std430, binding = 0) readonly buffer InBuf { Point inPoints[]; };
// 绑定点 1: 过滤后的紧凑数据
layout(std430, binding = 1) writeonly buffer OutBuf { Point outPoints[]; };
// 绑定点 2: 绘制指令 (CPU 通过 glDrawArraysIndirect 读取)
layout(std430, binding = 2) buffer IndirectBuf {
    uint vertexCount;   // 对应 glDrawArrays 的 count
    uint instanceCount; // 必须为 1
    uint firstVertex;   // 0
    uint baseInstance;  // 0
} indirect;

uniform vec3 uEgoPos;
uniform uint uTotalPoints;

shared uint groupCount;
shared uint groupOffset;
shared uint localIndices[256];

void main() {
    uint idx = gl_GlobalInvocationID.x;
    uint localIdx = gl_LocalInvocationID.x;

    // 1. 初始化组内共享变量
    if (localIdx == 0) groupCount = 0;
    barrier();

    // 2. 过滤逻辑 (距离判断 + 分级抽稀)
    bool keep = false;
    if (idx < uTotalPoints) {
        float d = distance(inPoints[idx].pos.xyz, uEgoPos);
        if (d < 3.5 || (d < 20 && idx % 3 == 0) || (d < 38.5 && idx % 15 == 0) ||
            (d < 49.5 && idx % 45 == 0) || (d < 66 && idx % 170 == 0) ||
            (d < 96 && idx % 300 == 0) || (d < 123 && idx % 700 == 0) ||
            (d < 150.0 && (idx % 1000 == 0))) {
            keep = true;
        }
    }

    // 3. 组内原子累加（获取组内相对偏移）
    uint myLocalOffset;
    if (keep) {
        myLocalOffset = atomicAdd(groupCount, 1);
        localIndices[myLocalOffset] = idx;
    }
    barrier();

    // 4. 由一个线程代表全组申请全局写入空间
    if (localIdx == 0 && groupCount > 0) {
        groupOffset = atomicAdd(indirect.vertexCount, groupCount);
    }
    barrier();

    // 5. 批量写入 OutBuf
    if (keep) {
        uint writeIdx = groupOffset + myLocalOffset;
        if (writeIdx < uTotalPoints) {
            outPoints[writeIdx] = inPoints[localIndices[myLocalOffset]];
        }
    }
}
```

### 2.2 渲染 Shader

`shaders/point_cloud.vert`：

```glsl
#version 430 core
layout(location = 0) in vec4 aPos;
layout(location = 1) in vec4 aColor;

uniform mat4 uProjection;
uniform mat4 uView;
uniform vec3 uEgoWorld; // 本帧实时车辆位置：把传感器相对坐标摆进世界
out vec4 vColor;

void main() {
    gl_Position = uProjection * uView * vec4(aPos.xyz + uEgoWorld, 1.0);
    vColor = aColor;
    gl_PointSize = 2.0;
}
```

`shaders/point_cloud.frag`：

```glsl
#version 430 core
in vec4 vColor;
out vec4 FragColor;
void main() { FragColor = vColor; }
```

### 2.3 资源初始化（构造函数核心）

```cpp
// 1. 三块 Input SSBO：Persistent + Coherent Mapping，全部入 freeList
GLbitfield flags = GL_MAP_WRITE_BIT | GL_MAP_PERSISTENT_BIT | GL_MAP_COHERENT_BIT;
glGenBuffers(NUM_BUFFERS, inputSSBO);
for (int i = 0; i < NUM_BUFFERS; ++i) {
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, inputSSBO[i]);
    glBufferStorage(GL_SHADER_STORAGE_BUFFER, pointCount * sizeof(Point), nullptr, flags);
    mappedPtr[i] = (Point*)glMapBufferRange(GL_SHADER_STORAGE_BUFFER, 0,
                                            pointCount * sizeof(Point), flags);
    freeList.push_back(i);
}

// 2. Output SSBO：纯 GPU 显存，不映射
glGenBuffers(1, &outputSSBO);
glBindBuffer(GL_SHADER_STORAGE_BUFFER, outputSSBO);
glBufferData(GL_SHADER_STORAGE_BUFFER, pointCount * sizeof(Point), nullptr, GL_STREAM_DRAW);

// 3. Indirect / 计数 Buffer：DrawArraysIndirectCommand 布局
glGenBuffers(1, &indirectBuffer);
glBindBuffer(GL_DRAW_INDIRECT_BUFFER, indirectBuffer);
DrawArraysIndirectCommand cmd = { 0, 1, 0, 0 };
glBufferData(GL_DRAW_INDIRECT_BUFFER, sizeof(cmd), &cmd, GL_DYNAMIC_DRAW);

uEgoPosLoc       = glGetUniformLocation(filterShader->ID, "uEgoPos");
uTotalPointsLoc  = glGetUniformLocation(filterShader->ID, "uTotalPoints");
```

### 2.4 Compute 派发（runFilter）

```cpp
void PointCloudLayer::runFilter(int index, const glm::vec3& egoPos, uint32_t count) {
    // 重置 indirect 指令：vertexCount 清零, instanceCount=1
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, indirectBuffer);
    uint32_t resetData[4] = { 0, 1, 0, 0 };
    glBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, sizeof(resetData), resetData);

    filterShader->use();
    glm::vec3 sensorOrigin(0.0f);
    glUniform3fv(uEgoPosLoc, 1, &sensorOrigin[0]);
    glUniform1ui(uTotalPointsLoc, (GLuint)count);

    // ★ 扁平绑定：binding 0/1/2 与 filter.comp 一一对应
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, inputSSBO[index]);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, outputSSBO);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, indirectBuffer);

    glDispatchCompute((count + 255) / 256, 1, 1);

    // 屏障：compute 写完后，indirect 读取 / 顶点属性读取才安全
    glMemoryBarrier(GL_COMMAND_BARRIER_BIT |
                    GL_SHADER_STORAGE_BARRIER_BIT |
                    GL_VERTEX_ATTRIB_ARRAY_BARRIER_BIT);

    // 打 fence：GPU 真正读完此输入缓冲后才 signaled → worker 不会覆写在读的数据
    if (fences[index]) glDeleteSync(fences[index]);
    fences[index] = glFenceSync(GL_SYNC_GPU_COMMANDS_COMPLETE, 0);
}
```

### 2.5 Indirect 绘制（draw）

```cpp
void PointCloudLayer::draw(const glm::mat4& view, const glm::mat4& projection,
                           const glm::vec3& egoWorld) {
    renderShader->use();
    renderShader->setMat4("uView", view);
    renderShader->setMat4("uProjection", projection);
    renderShader->setVec3("uEgoWorld", egoWorld);

    glBindVertexArray(vao);
    glBindBuffer(GL_ARRAY_BUFFER, outputSSBO); // 把 compute 的输出当顶点数据读

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, sizeof(Point), (void*)offsetof(Point, pos));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, sizeof(Point), (void*)offsetof(Point, color));

    glBindBuffer(GL_DRAW_INDIRECT_BUFFER, indirectBuffer);
    glDrawArraysIndirect(GL_POINTS, (void*)0); // count 来自 GPU 写入的 indirect buffer

    glBindBuffer(GL_DRAW_INDIRECT_BUFFER, 0);
    glBindVertexArray(0);
}
```

> 关键点：`outputSSBO` 既是 compute 的写出目标（SSBO），又是绘制时的顶点源（ARRAY_BUFFER）。同一块 buffer 双重身份，是 OpenGL 里 GPU-driven 渲染的常用手法。

---

## 3. 实现二：React + WGSL（裸 WebGPU）完整实现

直接用浏览器原生 WebGPU API，最接近 C++ 心智模型，能看清每一个 buffer / bind group / pipeline。

### 3.1 WGSL 着色器（compute + render 三段）

```ts
// shaders.ts
export const filterWGSL = /* wgsl */ `
struct Point {
  pos:   vec4<f32>,
  color: vec4<f32>,
};

struct Indirect {
  vertexCount:   atomic<u32>,   // ← C++ 里的普通 uint，这里必须 atomic
  instanceCount: u32,
  firstVertex:   u32,
  baseInstance:  u32,
};

struct Params {
  egoPos:      vec3<f32>,
  totalPoints: u32,
};

@group(0) @binding(0) var<storage, read>       inPoints  : array<Point>;
@group(0) @binding(1) var<storage, read_write> outPoints : array<Point>;
@group(0) @binding(2) var<storage, read_write> indirect  : Indirect;
@group(0) @binding(3) var<uniform>             params    : Params;

var<workgroup> groupCount: atomic<u32>;
var<workgroup> groupOffset: u32;
var<workgroup> localIndices: array<u32, 256>;

@compute @workgroup_size(256)
fn main(@builtin(global_invocation_id) gid: vec3<u32>,
        @builtin(local_invocation_id)  lid: vec3<u32>) {
  let idx = gid.x;
  let localIdx = lid.x;

  // 1. 初始化共享变量
  if (localIdx == 0u) { atomicStore(&groupCount, 0u); }
  workgroupBarrier();

  // 2. 过滤逻辑
  var keep = false;
  if (idx < params.totalPoints) {
    let d = distance(inPoints[idx].pos.xyz, params.egoPos);
    if (d < 3.5 || (d < 20.0 && idx % 3u == 0u) || (d < 38.5 && idx % 15u == 0u) ||
        (d < 49.5 && idx % 45u == 0u) || (d < 66.0 && idx % 170u == 0u) ||
        (d < 96.0 && idx % 300u == 0u) || (d < 123.0 && idx % 700u == 0u) ||
        (d < 150.0 && idx % 1000u == 0u)) {
      keep = true;
    }
  }

  // 3. 组内原子累加
  var myLocalOffset = 0u;
  if (keep) {
    myLocalOffset = atomicAdd(&groupCount, 1u);  // ← 传指针 &
    localIndices[myLocalOffset] = idx;
  }
  workgroupBarrier();

  // 4. 代表线程申请全局偏移
  if (localIdx == 0u && atomicLoad(&groupCount) > 0u) {
    groupOffset = atomicAdd(&indirect.vertexCount, atomicLoad(&groupCount));
  }
  workgroupBarrier();

  // 5. 批量写出
  if (keep) {
    let writeIdx = groupOffset + myLocalOffset;
    if (writeIdx < params.totalPoints) {
      outPoints[writeIdx] = inPoints[localIndices[myLocalOffset]];
    }
  }
}
`;

export const renderWGSL = /* wgsl */ `
struct Camera { viewProj: mat4x4<f32>, egoWorld: vec3<f32> };
@group(0) @binding(0) var<uniform> camera: Camera;

struct Point { pos: vec4<f32>, color: vec4<f32> };
@group(0) @binding(1) var<storage, read> points: array<Point>;

struct VSOut {
  @builtin(position) clip: vec4<f32>,
  @location(0) color: vec4<f32>,
};

@vertex
fn vs(@builtin(vertex_index) vi: u32) -> VSOut {
  let p = points[vi];
  var out: VSOut;
  out.clip = camera.viewProj * vec4<f32>(p.pos.xyz + camera.egoWorld, 1.0);
  out.color = p.color;
  return out;
}

@fragment
fn fs(in: VSOut) -> @location(0) vec4<f32> {
  return in.color;
}
`;
```

### 3.2 React 组件：初始化 + 每帧调度

```tsx
// PointCloudWebGPU.tsx
import { useEffect, useRef } from "react";
import { mat4 } from "gl-matrix";
import { filterWGSL, renderWGSL } from "./shaders";

const POINT_COUNT = 1_000_000;
const POINT_STRIDE = 8 * 4; // 2x vec4<f32> = 32 bytes
const WORKGROUP = 256;

export function PointCloudWebGPU() {
  const canvasRef = useRef<HTMLCanvasElement>(null);

  useEffect(() => {
    let raf = 0;
    let disposed = false;

    (async () => {
      const canvas = canvasRef.current!;
      const adapter = await navigator.gpu.requestAdapter();
      const device = await adapter!.requestDevice();
      const ctx = canvas.getContext("webgpu")!;
      const format = navigator.gpu.getPreferredCanvasFormat();
      ctx.configure({ device, format, alphaMode: "opaque" });

      // ---- Buffers（对应 C++ 的 inputSSBO/outputSSBO/indirectBuffer）----
      const inputBuffer = device.createBuffer({
        size: POINT_COUNT * POINT_STRIDE,
        usage: GPUBufferUsage.STORAGE | GPUBufferUsage.COPY_DST,
      });
      const outputBuffer = device.createBuffer({
        size: POINT_COUNT * POINT_STRIDE,
        // STORAGE: compute 写; 同时当顶点源在 vs 里按 storage 读
        usage: GPUBufferUsage.STORAGE,
      });
      const indirectBuffer = device.createBuffer({
        size: 4 * 4, // 4 x u32
        usage: GPUBufferUsage.INDIRECT | GPUBufferUsage.STORAGE | GPUBufferUsage.COPY_DST,
      });
      const paramsBuffer = device.createBuffer({
        size: 16 + 4 + 12, // vec3 + u32, 注意对齐补到 32
        usage: GPUBufferUsage.UNIFORM | GPUBufferUsage.COPY_DST,
      });
      const cameraBuffer = device.createBuffer({
        size: 64 + 16, // mat4x4 + vec3(补齐)
        usage: GPUBufferUsage.UNIFORM | GPUBufferUsage.COPY_DST,
      });

      // 用一批假数据填充输入（实际项目里换成你的雷达点）
      const init = new Float32Array(POINT_COUNT * 8);
      for (let i = 0; i < POINT_COUNT; i++) {
        const o = i * 8;
        init[o + 0] = (Math.random() - 0.5) * 300; // x
        init[o + 1] = (Math.random() - 0.5) * 20;  // y
        init[o + 2] = (Math.random() - 0.5) * 300; // z
        init[o + 4] = 0.2; init[o + 5] = 0.8; init[o + 6] = 1.0; init[o + 7] = 1; // color
      }
      device.queue.writeBuffer(inputBuffer, 0, init);

      // ---- Compute pipeline ----
      const computeModule = device.createShaderModule({ code: filterWGSL });
      const computePipeline = device.createComputePipeline({
        layout: "auto",
        compute: { module: computeModule, entryPoint: "main" },
      });
      // ★ 两级绑定：@group(0) 的整组打包成一个 BindGroup
      const computeBindGroup = device.createBindGroup({
        layout: computePipeline.getBindGroupLayout(0),
        entries: [
          { binding: 0, resource: { buffer: inputBuffer } },
          { binding: 1, resource: { buffer: outputBuffer } },
          { binding: 2, resource: { buffer: indirectBuffer } },
          { binding: 3, resource: { buffer: paramsBuffer } },
        ],
      });

      // ---- Render pipeline ----
      const renderModule = device.createShaderModule({ code: renderWGSL });
      const renderPipeline = device.createRenderPipeline({
        layout: "auto",
        vertex: { module: renderModule, entryPoint: "vs" },
        fragment: { module: renderModule, entryPoint: "fs", targets: [{ format }] },
        primitive: { topology: "point-list" },
      });
      const renderBindGroup = device.createBindGroup({
        layout: renderPipeline.getBindGroupLayout(0),
        entries: [
          { binding: 0, resource: { buffer: cameraBuffer } },
          { binding: 1, resource: { buffer: outputBuffer } }, // ← 复用 compute 的输出
        ],
      });

      const frame = (t: number) => {
        if (disposed) return;

        // 1. 更新 params（egoPos + totalPoints）
        const params = new ArrayBuffer(32);
        new Float32Array(params, 0, 3).set([0, 0, 0]);   // egoPos
        new Uint32Array(params, 16, 1).set([POINT_COUNT]); // totalPoints
        device.queue.writeBuffer(paramsBuffer, 0, params);

        // 2. 更新相机
        const aspect = canvas.width / canvas.height;
        const proj = mat4.perspective(mat4.create(), Math.PI / 3, aspect, 0.1, 1000);
        const view = mat4.lookAt(mat4.create(), [0, 60, 160], [0, 0, 0], [0, 1, 0]);
        const vp = mat4.multiply(mat4.create(), proj, view);
        const cam = new Float32Array(80 / 4);
        cam.set(vp, 0);
        cam.set([0, 0, 0], 16); // egoWorld
        device.queue.writeBuffer(cameraBuffer, 0, cam);

        // 3. 重置 indirect: [vertexCount=0, instanceCount=1, first=0, base=0]
        device.queue.writeBuffer(indirectBuffer, 0, new Uint32Array([0, 1, 0, 0]));

        const encoder = device.createCommandEncoder();

        // 4. Compute pass（对应 glDispatchCompute）
        {
          const pass = encoder.beginComputePass();
          pass.setPipeline(computePipeline);
          pass.setBindGroup(0, computeBindGroup);           // ← setBindGroup(group索引, 整组)
          pass.dispatchWorkgroups(Math.ceil(POINT_COUNT / WORKGROUP));
          pass.end();
        }
        // 注意：WebGPU 不需要手动 glMemoryBarrier，pass 之间自动插入依赖屏障

        // 5. Render pass + indirect draw（对应 glDrawArraysIndirect）
        {
          const pass = encoder.beginRenderPass({
            colorAttachments: [{
              view: ctx.getCurrentTexture().createView(),
              clearValue: { r: 0.05, g: 0.05, b: 0.08, a: 1 },
              loadOp: "clear", storeOp: "store",
            }],
          });
          pass.setPipeline(renderPipeline);
          pass.setBindGroup(0, renderBindGroup);
          pass.drawIndirect(indirectBuffer, 0);  // count 来自 GPU 写入的 buffer
          pass.end();
        }

        device.queue.submit([encoder.finish()]);
        raf = requestAnimationFrame(frame);
      };
      raf = requestAnimationFrame(frame);
    })();

    return () => { disposed = true; cancelAnimationFrame(raf); };
  }, []);

  return <canvas ref={canvasRef} width={1280} height={720} />;
}
```

> **与 C++ 的核心差异回顾**：
> - 绑定从「逐个 `glBindBufferBase`」变成「一次 `createBindGroup` 打包 + `setBindGroup`」。
> - 没有 `glMemoryBarrier`：compute pass 写 `outputBuffer`，render pass 读它，WebGPU 自动加依赖屏障。
> - `vertexCount` 在 WGSL 里必须是 `atomic<u32>`，C++ 里是普通 `uint`。
> - indirect buffer 必须显式声明 `GPUBufferUsage.INDIRECT`。

---

## 4. 实现三：React + Three.js WebGPURenderer 完整实现

Three.js 的 `WebGPURenderer` + **TSL（Three Shading Language，JS 写 shader）**。你不直接写 WGSL，而是用 JS 节点描述计算，Three.js 在底层编译成 WGSL 并自动管理 buffer / bind group / pipeline。抽象层级最高，代码最短，但「绑定点」「barrier」这些概念几乎被隐藏。

### 4.1 React 组件（用 `@react-three/fiber` + TSL compute）

```tsx
// PointCloudThree.tsx
import { Canvas, useFrame, useThree, extend } from "@react-three/fiber";
import * as THREE from "three/webgpu";
import {
  Fn, instancedArray, instanceIndex, distance as tslDistance,
  vec3, float, uint, If, uniform,
} from "three/tsl";
import { useMemo, useRef } from "react";

const POINT_COUNT = 1_000_000;

function PointCloud() {
  const { gl } = useThree(); // gl 实际是 WebGPURenderer
  const renderer = gl as unknown as THREE.WebGPURenderer;

  const { mesh, computeFilter, egoUniform } = useMemo(() => {
    // ---- Storage buffers（对应 C++ 的 SSBO；Three 用 instancedArray 管理）----
    // 输入：每个点的位置 + 颜色
    const inPos   = instancedArray(POINT_COUNT, "vec3");
    const inColor = instancedArray(POINT_COUNT, "vec3");
    // 输出：紧凑后的位置 + 颜色（绘制时直接当顶点属性）
    const outPos   = instancedArray(POINT_COUNT, "vec3");
    const outColor = instancedArray(POINT_COUNT, "vec3");
    // 计数器：相当于 indirect.vertexCount（这里用一个长度 1 的 atomic storage）
    const counter = instancedArray(1, "uint").toAtomic();

    const egoUniform = uniform(vec3(0, 0, 0));

    // ---- 用 JS 初始化输入数据 ----
    const initCompute = Fn(() => {
      const i = instanceIndex;
      // 伪随机散布（演示用）
      const fi = float(i);
      inPos.element(i).assign(vec3(
        fi.mul(12.9898).sin().mul(43758.5453).fract().sub(0.5).mul(300),
        fi.mul(78.233).sin().mul(12345.678).fract().sub(0.5).mul(20),
        fi.mul(37.719).sin().mul(98765.43).fract().sub(0.5).mul(300),
      ));
      inColor.element(i).assign(vec3(0.2, 0.8, 1.0));
    })().compute(POINT_COUNT);
    renderer.computeAsync(initCompute);

    // ---- 过滤 compute（对应 filter.comp）----
    // 注意：TSL 没有暴露 workgroup 共享内存做组内压缩，这里用全局 atomic 直接抢位，
    // 算法等价（流压缩），只是少了 C++ 里「组内先汇总再申请」的两级优化。
    const computeFilter = Fn(() => {
      const i = instanceIndex;
      const d = tslDistance(inPos.element(i), egoUniform);

      const keep = d.lessThan(3.5)
        .or(d.lessThan(20).and(i.mod(uint(3)).equal(uint(0))))
        .or(d.lessThan(38.5).and(i.mod(uint(15)).equal(uint(0))))
        .or(d.lessThan(49.5).and(i.mod(uint(45)).equal(uint(0))))
        .or(d.lessThan(66).and(i.mod(uint(170)).equal(uint(0))))
        .or(d.lessThan(96).and(i.mod(uint(300)).equal(uint(0))))
        .or(d.lessThan(123).and(i.mod(uint(700)).equal(uint(0))))
        .or(d.lessThan(150).and(i.mod(uint(1000)).equal(uint(0))));

      If(keep, () => {
        // 全局原子抢一个写入下标（对应 atomicAdd(indirect.vertexCount, 1)）
        const w = counter.element(0).atomicAdd(uint(1));
        outPos.element(w).assign(inPos.element(i));
        outColor.element(w).assign(inColor.element(i));
      });
    })().compute(POINT_COUNT);

    // ---- 渲染网格：用 outPos/outColor 作为顶点属性 ----
    const geometry = new THREE.BufferGeometry();
    // 占位 position 属性，真正坐标由 TSL positionNode 提供
    geometry.setAttribute(
      "position",
      new THREE.BufferAttribute(new Float32Array(POINT_COUNT * 3), 3),
    );

    const material = new THREE.PointsNodeMaterial();
    material.positionNode = outPos.toAttribute().add(egoUniform); // 摆进世界
    material.colorNode = outColor.toAttribute();
    material.size = 2;

    const mesh = new THREE.Points(geometry, material);
    mesh.frustumCulled = false;
    return { mesh, computeFilter, egoUniform };
  }, [renderer]);

  useFrame(() => {
    // 每帧：清零计数器 + 跑过滤 compute
    // （Three 没有现成的「indirect draw count 直连」，常见做法是固定画 POINT_COUNT，
    //   被剔除的点在 compute 里写成退化/零，或用 counter 控制 drawRange。）
    renderer.computeAsync(computeFilter);
  });

  return <primitive object={mesh} />;
}

export function PointCloudThree() {
  return (
    <Canvas
      gl={(props) => new THREE.WebGPURenderer(props as any)} // 关键：用 WebGPURenderer
      camera={{ position: [0, 60, 160], fov: 60, far: 1000 }}
    >
      <PointCloud />
    </Canvas>
  );
}
```

> **Three.js 版的取舍**：
> - **优点**：不写一行 WGSL，buffer / bind group / pipeline / barrier 全部由 Three 自动生成；和 R3F 生态、相机控制、后处理无缝衔接。
> - **代价**：`@group/@binding`、`workgroupBarrier`、`drawIndirect` 这些概念被封装掉了，学不到底层。TSL 目前**不直接暴露 workgroup 共享内存**，所以 C++ 里「组内两级压缩」难以原样复刻，一般退化为「全局 atomic 抢位」；indirect draw count 也不像裸 WebGPU 那样能直接驱动 `drawIndirect`，常用 `geometry.setDrawRange()` 或把剔除点写成退化点替代。
> - **结论**：适合做产品 / 快速可视化；想深入理解 compute 管线，还是裸 WebGPU 更直观。

---

## 5. 三端横向对比表

| 维度 | C++ / OpenGL 4.3 | React + WGSL (裸 WebGPU) | React + Three.js (WebGPURenderer) |
|---|---|---|---|
| 着色器语言 | GLSL 430 | WGSL | TSL（JS 写，编译成 WGSL） |
| 绑定模型 | 一级 `binding=N`，`glBindBufferBase` 逐个绑 | 两级 `@group/@binding`，`BindGroup` 打包 | 隐藏，`instancedArray` 自动管理 |
| 工作组大小 | `local_size_x=256` | `@workgroup_size(256)` | `.compute(count)`，自动分组 |
| 共享内存 / 组内压缩 | `shared` + `barrier()`，两级原子 | `var<workgroup>` + `workgroupBarrier()` | **不直接暴露**，退化为全局 atomic |
| 原子计数器 | 普通 `uint` + `atomicAdd` | 必须 `atomic<u32>` + `atomicAdd(&x,..)` | `.toAtomic()` + `.atomicAdd()` |
| 内存屏障 | 手动 `glMemoryBarrier(...)` | 自动（pass 间依赖） | 自动 |
| Indirect draw | `glDrawArraysIndirect` + 手搓 cmd buffer | `pass.drawIndirect(buf, 0)`，buffer 需 `INDIRECT` usage | 一般用 `setDrawRange` / 退化点替代 |
| 输出复用为顶点源 | 同一 `outputSSBO` 既是 SSBO 又是 VBO | `outputBuffer` 既 STORAGE 又在 vs 里 storage 读 | `outPos.toAttribute()` |
| CPU/GPU 异步 | 手搓三缓冲 + `glFenceSync` | `mapAsync` / 多 buffer，Promise 化 | renderer 内部托管 |
| 代码量 / 抽象 | 最多，全手动 | 中等，概念清晰 | 最少，概念被封装 |
| 学习价值 | 理解 GPU 工作机制最佳 | 理解现代绑定模型最佳 | 快速出活、生态最佳 |

### 迁移建议

- 从你的 OpenGL 代码迁到 Web：**先走裸 WebGPU（实现二）**，因为算法、binding、indirect draw 几乎一一对应，迁移成本最低、能验证理解是否正确。
- 跑通后若要做产品（接相机控制、后处理、加载模型），再考虑 **Three.js WebGPURenderer（实现三）**，把底层细节交给框架。
- 三端在「compute 过滤 → 原子压缩 → 画出来」这条主线上**算法完全一致**，区别只在绑定写法、屏障是否手动、indirect 是否原生支持这三处。

---

## 6. 进阶：多分辨率体素栅格 LOD（八叉树分层的「扁平等价物」）

### 6.1 为什么不用真八叉树（针对 10–20Hz 流式点云）

八叉树 LOD（Potree / 3D Tiles）前提是**数据静态**：离线建一次树、之后只按相机距离选层。流式 LiDAR 每帧空间分布全变 → 每帧重建树（递归分裂、指针管理、CPU↔GPU 同步）吃满帧预算（20Hz=50ms / 10Hz=100ms），且树结构对 GPU 并行极不友好。

**一层八叉树 ≈ 一个体素栅格。** 用「多个分辨率的体素栅格、按距离选用」就能拿到"近密远稀"的分层效果，而它的「每帧重建」退化成：

1. 清空一块定长哈希表（`glClearBufferData` / 一个 clear compute，O(表大小)）
2. 一趟 O(N) compute：每点哈希到体素，原子抢占，每体素只留一个代表点

无递归、无指针、无排序、无 CPU 同步 → 重建几乎免费，100 万点通常几百 µs ~ 个位数 ms。

| | 真八叉树（每帧重建） | 体素哈希表 |
|---|---|---|
| 每帧"重建" | 递归建树 + 上传，CPU 重活 | 清一块定长数组 |
| 主体计算 | 难并行 | O(N) compute，完美并行 |
| CPU↔GPU 同步 | 有 | 无 |
| 分层 LOD | ✅ | ✅（`voxelSize(d)` 分档） |

### 6.2 GLSL 版（项目内实际文件 `shaders/filter_voxel.comp`）

核心：在旧 `filter.comp` 基础上，把 `keep` 的判断从 `idx % N` 换成「体素去重」，并新增**绑定点 3**（体素哈希表）。其余（组内压缩、indirect 输出）完全不变。

```glsl
layout(std430, binding = 3) buffer VoxelTable { uint voxelTable[]; }; // 每帧清成 0xFFFFFFFF

const uint EMPTY = 0xFFFFFFFFu;

float voxelSize(float d) {            // 距离 → 体素边长 = 「分层」
    if (d < 20.0) return 0.2;
    else if (d < 50.0) return 0.5;
    else if (d < 100.0) return 1.5;
    else return 4.0;
}
uint packVoxel(ivec3 c) {             // 体素坐标打包成唯一 key（各轴 ±512）
    uvec3 u = uvec3(c + 512);
    return (u.x & 0x3FFu) | ((u.y & 0x3FFu) << 10) | ((u.z & 0x3FFu) << 20);
}
bool claimVoxel(uint key) {           // CAS + 线性探测：抢到空槽=代表点
    uint start = (key * 2654435761u) % uTableSize;
    for (uint p = 0u; p < 32u; ++p) {
        uint s = (start + p) % uTableSize;
        uint prev = atomicCompSwap(voxelTable[s], EMPTY, key);
        if (prev == EMPTY) return true;   // 抢到 → 保留
        if (prev == key)   return false;  // 同体素已占 → 丢弃
    }
    return true; // 表太满，保守保留
}
// main() 里：keep = (d <= uMaxRange) && claimVoxel(packVoxel(ivec3(floor(p / voxelSize(d)))));
```

C++ 集成：新增 `voxelTableBuffer`，每帧 `glClearBufferData(..., 0xFFFFFFFF)` 清空，`glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 3, voxelTableBuffer)`，并设 `uTableSize`（≈ 预期输出点数 ×3）、`uMaxRange`。

### 6.3 WGSL 版（裸 WebGPU）

WGSL 没有 `atomicCompSwap`，对应 `atomicCompareExchangeWeak`（返回 `{old_value, exchanged}`）。体素表声明为 `array<atomic<u32>>`。

```wgsl
struct Params { egoPos: vec3<f32>, totalPoints: u32, tableSize: u32, maxRange: f32 };

@group(0) @binding(0) var<storage, read>       inPoints  : array<Point>;
@group(0) @binding(1) var<storage, read_write> outPoints : array<Point>;
@group(0) @binding(2) var<storage, read_write> indirect  : Indirect;       // vertexCount: atomic<u32>
@group(0) @binding(3) var<storage, read_write> voxelTable: array<atomic<u32>>; // ← 新增
@group(0) @binding(4) var<uniform>             params    : Params;

const EMPTY : u32 = 0xFFFFFFFFu;

fn voxelSize(d: f32) -> f32 {
  if (d < 20.0) { return 0.2; } else if (d < 50.0) { return 0.5; }
  else if (d < 100.0) { return 1.5; } return 4.0;
}
fn packVoxel(c: vec3<i32>) -> u32 {
  let u = vec3<u32>(c + vec3<i32>(512));
  return (u.x & 0x3FFu) | ((u.y & 0x3FFu) << 10u) | ((u.z & 0x3FFu) << 20u);
}
fn claimVoxel(key: u32) -> bool {
  let start = (key * 2654435761u) % params.tableSize;
  for (var p: u32 = 0u; p < 32u; p = p + 1u) {
    let s = (start + p) % params.tableSize;
    let res = atomicCompareExchangeWeak(&voxelTable[s], EMPTY, key);
    if (res.exchanged) { return true; }       // 抢到空槽 → 保留
    if (res.old_value == key) { return false; } // 同体素已占 → 丢弃
    // 注意 weak 版可能伪失败：old_value==EMPTY 但 exchanged==false → 继续探测，正确性不受影响
  }
  return true;
}
// main() 同 1.2 节结构，keep = (d <= maxRange) && claimVoxel(packVoxel(...));
```

体素表清空：`encoder.clearBuffer` 只能清 0，所以另写一个 clear compute 每帧先跑：

```wgsl
@group(0) @binding(0) var<storage, read_write> voxelTable: array<atomic<u32>>;
@compute @workgroup_size(256)
fn clear(@builtin(global_invocation_id) gid: vec3<u32>) {
  if (gid.x < arrayLength(&voxelTable)) { atomicStore(&voxelTable[gid.x], 0xFFFFFFFFu); }
}
```

JS 端每帧顺序：`clear pass` → `filter pass` → `render pass(drawIndirect)`，三个 pass 之间 WebGPU 自动插依赖屏障。

### 6.4 Three.js TSL 版（2-pass atomicMax「重要性赢家」）

TSL 对 CAS + 线性探测哈希表支持不友好（且 atomics API 仍在演进，需按 three 版本核对）。这里换一个更 TSL-友好、且**顺带实现"重要性 LOD"**的 2-pass 写法：每个体素保留 **score 最高**的那个点。

```ts
import {
  Fn, instancedArray, instanceIndex, uint, float, floor, vec3, If,
  distance as tslDistance,
} from "three/tsl";

const TABLE = 1 << 20;
// 槽里存打包值：高 12 位 = score，低 20 位 = 点下标。atomicMax → score 最高者胜。
const voxelWinner = instancedArray(TABLE, "uint").toAtomic(); // 每帧清 0

const hashSlot = (key) => key.mul(uint(2654435761)).mod(uint(TABLE));
const packVoxel = (c) => /* 同 GLSL：各轴 +512 后拼成 key */ /* ... */ key;

// score：常数=纯空间均匀；换成强度梯度/边缘度 = 重要性 LOD（接你的「概率密度」问题）
const computeImportance = (i) => uint(1); // TODO: 替换为真实重要性

// Pass A —— 投票：每点把 (score<<20 | index) atomicMax 进自己体素的槽
const votePass = Fn(() => {
  const i = instanceIndex;
  const p = inPos.element(i);
  const d = tslDistance(p, ego);
  If(d.lessThanEqual(maxRange), () => {
    const vs = voxelSizeNode(d);
    const key = packVoxel(floor(p.div(vs)));
    const slot = hashSlot(key);
    const packed = computeImportance(i).shiftLeft(uint(20)).bitOr(i.bitAnd(uint(0xFFFFF)));
    voxelWinner.element(slot).atomicMax(packed);
  });
})().compute(POINT_COUNT);

// Pass B —— 落地：若本点是所在体素的赢家，则原子压缩写出
const emitPass = Fn(() => {
  const i = instanceIndex;
  const p = inPos.element(i);
  const d = tslDistance(p, ego);
  If(d.lessThanEqual(maxRange), () => {
    const slot = hashSlot(packVoxel(floor(p.div(voxelSizeNode(d)))));
    const winnerIdx = voxelWinner.element(slot).bitAnd(uint(0xFFFFF));
    If(winnerIdx.equal(i.bitAnd(uint(0xFFFFF))), () => {
      const w = counter.element(0).atomicAdd(uint(1)); // 紧凑下标
      outPos.element(w).assign(p);
      outColor.element(w).assign(inColor.element(i));
    });
  });
})().compute(POINT_COUNT);

// 每帧：清 voxelWinner+counter → renderer.computeAsync(votePass) → computeAsync(emitPass) → 渲染
```

> **2-pass atomicMax 的取舍**：只需 `atomicMax`（TSL 稳定支持），无需 CAS/探测；天然支持"重要性赢家"。代价是**哈希冲突**（两个不同体素映射到同一槽时，只有一个体素的赢家存活，另一体素整体被丢）——可视化场景可接受，加大 `TABLE` 可降低概率。CAS+探测版（GLSL/WGSL）则精确无误丢。

### 6.5 体素 LOD 三端对比

| 维度 | GLSL | WGSL | Three.js TSL |
|---|---|---|---|
| 去重算法 | CAS + 线性探测（精确） | CAS + 线性探测（精确） | 2-pass atomicMax（有冲突近似） |
| 原子原语 | `atomicCompSwap` | `atomicCompareExchangeWeak` | `atomicMax` |
| 体素表清空 | `glClearBufferData` | clear compute（clearBuffer 只能清 0） | 清 `instancedArray` |
| 重要性 LOD | 需改造（额外存 score） | 需改造 | **天然支持**（score 即赢家依据） |
| Pass 数 | 1 | 1（+1 clear） | 2（vote + emit） |

### 6.6 重要性 LOD 变体（GLSL，文件 `filter_voxel_importance.comp`）

把 6.2 的「CAS 先到先得」换成「**score 最高者占坑**」，即每个体素保留最重要的点。

- **score 来源**：`inPoints[idx].pos.w`（`Point` 结构早已预留 `pos.w = intensity`，零额外显存）。
- **打包**：`packedKey = (score << 24) | (idx & 0xFFFFFF)`，score 占高 8 位、idx 占低 24 位（支持 ≤16M 点）。`atomicMax` 比较整 32 位 → score 高位主导。
  - ⚠️ **不能用 `packed` 当变量名**——它是 GLSL 保留关键字，会报 `unexpected PACKED_TOK`。
- **2-pass**：Pass 0 每点 `atomicMax(voxelTable[slot], packedKey)` 投票；Pass 1 若 `voxelTable[slot] 低24位 == 自己 idx` 则保留并压缩写出。两趟之间加 `glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT)`。
- 与 CAS 版差别只两处：**体素表清 0**（非 `0xFFFFFFFF`）；**dispatch 两趟**。

```glsl
// 关键片段
uint score     = uint(clamp(inPoints[idx].pos.w, 0.0, 1.0) * 255.0);
uint packedKey = (score << 24) | (idx & 0xFFFFFFu);
if (uPass == 0u) { atomicMax(voxelTable[slot], packedKey); return; }   // 投票
// Pass 1: keep = (voxelTable[slot] & 0xFFFFFFu) == (idx & 0xFFFFFFu); 再走组内压缩
```

### 6.7 score 从哪来：有组织点云的上游曲率 pass（文件 `gradient.comp`）

6.6 消费 `pos.w`，但 `pos.w` 里的「梯度/曲率」得有人算。**梯度是邻域特征，必须在数据还「有组织」时算**——摊平成无序数组后就没邻居了。

- **有组织布局**：LiDAR 原始数据按 `ring × azimuth` 排成 range image，`idx = ring*W + col`。同一 ring 的点内存连续，**azimuth 邻居 = `idx ± k`（同 ring 取模）**，无需任何空间搜索结构。
- **LOAM 式曲率**：对同一 ring 左右各 S 个邻居求位置差之和的模长，按距离归一。平面区≈0，边/角区大。
- **运行顺序**：`gradient.comp`（写 `pos.w`）→ `glMemoryBarrier` → `filter_voxel_importance.comp`（读 `pos.w`）。
- **无读写冲突**：该 pass 只读邻居 `.xyz`、只写自己的 `.w`（同 vec4 不同 float 偏移）。

```glsl
// 关键片段：同 ring azimuth 邻居 → 曲率 → 写回 pos.w
for (uint k = 1u; k <= uHalfWin; ++k) {
    vec3 PL = points[ring*uWidth + (col + uWidth - k%uWidth) % uWidth].pos.xyz;
    vec3 PR = points[ring*uWidth + (col + k) % uWidth].pos.xyz;
    sumDiff += (PL - Pi) + (PR - Pi);
}
points[idx].pos.w = clamp(length(sumDiff) / (2*S*ri) * uCurvScale, 0.0, 1.0);
```

> **数据生成端的硬约束**：上游必须按 `idx = ring*W + col` 顺序写点，曲率 pass 才能用 `idx±k` 取到真邻居。随机散点（如当前 mock）算不出有意义的梯度——要么生成有组织数据，要么先用「高度 + 强度」当 score 跑通链路。

### 6.8 完整重要性 LOD 管线（C++ 端每帧顺序）

```
1. [worker] 生成有组织点云 (ring×azimuth 顺序) + 原始 intensity 入 pos.w
2. [render]  gradient.comp        : 算曲率，覆写 pos.w   →  barrier(SSBO)
3. [render]  清体素表为 0
4. [render]  filter_voxel_importance.comp uPass=0 (投票) →  barrier(SSBO)
5. [render]  filter_voxel_importance.comp uPass=1 (落地) →  barrier(CMD|SSBO|VATTR)
6. [render]  glDrawArraysIndirect  : 画保留下来的「重要点」
```

---

## 7. 用「概率密度 / 重要性」做 LOD：概念澄清

### 7.1 点云密度 ≠ 概率密度

真实 LiDAR 点的疏密**由传感器采样几何决定**，不是"物体/重要性"的概率密度：

- 随距离 **~1/d²** 衰减（光束发散，远处单位面积落点少）
- **入射角**：掠射表面点稀，正对密（×cosθ）
- **反射率 / 遮挡**：低反射、镜面、阴影区掉点

例：2m 外一面墙上万点，40m 外一个行人几个点。**密度表达的是"离传感器近"，不是"重要"。** 所以「按密度正比保留」会放大近处偏置，LOD 反而更糟。

### 7.2 但"密度驱动 LOD"是正确技术——看用什么变换

| 思路 | 概率 p | 效果 |
|---|---|---|
| **密度均衡**（体素栅格） | 每体素封顶 1 个 | 抹掉 1/d² 过采样，屏幕均匀。**本文第 6 节就是这个** |
| **逆密度概率保留** | `p ∝ 1/局部密度` | 输出均匀；Poisson-disk / 蓝噪声视觉最佳 |
| **重要性 / 显著性** | 边缘、曲率、强度梯度大处 p 高 | 保特征、抽平面，最利于感知/检测 |

### 7.3 流式数据（10–20Hz）的实践要点

1. **别用每帧纯随机概率保留**：同点这帧留下帧丢 → **时间闪烁**。体素栅格按空间格子去重，帧间稳定得多。
2. **体素栅格本身就是密度均衡 LOD**：它把过采样的近处压到目标密度，已经在做"密度驱动"，只是用"封顶"而非"正比"。
3. **要重要性 LOD**：把"第一个点占坑"改成"score 最高的点占坑"（即 6.4 的 2-pass atomicMax），`score` 用强度梯度 / 边缘度 / 是否地面点等。这才接近"信息密度"，比纯空间均匀更好。
4. KDE 等正式密度估计对实时太重；**体素占用就是局部密度的廉价代理**。

> 结论：**别把"点多"当"重要"**；但可以把"局部密度"当作要被均衡掉的过采样信号（体素封顶），再叠一个重要性权重决定每个格子留谁（atomicMax 赢家）。
