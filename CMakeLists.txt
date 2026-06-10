# cmake_minimum_required(VERSION 3.10)
# project(PointCloud)

# set(CMAKE_CXX_STANDARD 17)

# # 查找系统库
# find_package(glfw3 REQUIRED)
# find_package(OpenGL REQUIRED)

# # include 目录
# include_directories(
#     include
#     external/glad/include
# )

# # 可执行文件
# add_executable(app
#     src/main.cpp
#     external/glad/src/glad.c
# )

# # 链接库
# target_link_libraries(app
#     glfw
#     OpenGL::GL
#     dl
# )

cmake_minimum_required(VERSION 3.10)
set(OpenGL_GL_PREFERENCE GLVND)
project(PointCloud)

# --- 新增：强制默认使用 Release 模式 ---
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Choose the type of build." FORCE)
endif()

# --- 新增：针对 GCC/Clang 的优化参数 ---
if(MSVC)
    add_compile_options(/O2 /Ot) # Windows 优化
else()
    add_compile_options(-O3 -march=native -ffast-math) # Linux/Mac 极致优化
endif()

# 1. 强制 C++17
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 2. 查找库
# 1. 查找 TBB 库
find_package(TBB REQUIRED)
find_package(glfw3 REQUIRED)
find_package(OpenGL REQUIRED)
find_package(Threads REQUIRED)

set(IMGUI_DIR ${CMAKE_SOURCE_DIR}/external/imgui)

# 3. 设置 Include 路径
include_directories(
    include
    include/renderer
    include/data
    external/glad/include
    external/glm                # <--- glm 数学库（纯头文件，项目自带）
    ${IMGUI_DIR}               # <--- ImGui 核心头文件
    ${IMGUI_DIR}/backends      # <--- ImGui 后端头文件
)

# 4. 精确指定源码（避免 GLOB 抓取到多余的 main 函数）
# 建议手动列出文件，或者确保 src 目录下只有一个 main.cpp

# 4. 精确指定源码
# ---------------------------------------------------------
# 【新增】：收集 ImGui 的源文件
# ---------------------------------------------------------
set(IMGUI_SRC
    ${IMGUI_DIR}/imgui.cpp
    ${IMGUI_DIR}/imgui_draw.cpp
    ${IMGUI_DIR}/imgui_widgets.cpp
    ${IMGUI_DIR}/imgui_tables.cpp
    ${IMGUI_DIR}/imgui_demo.cpp
    ${IMGUI_DIR}/backends/imgui_impl_glfw.cpp
    ${IMGUI_DIR}/backends/imgui_impl_opengl3.cpp
)

set(SRC_LIST
    src/main.cpp
    src/renderer/Renderer.cpp
    src/renderer/PointCloudLayer.cpp
    src/data/MockFrameGenerator.cpp
    external/glad/src/glad.c  # <--- 关键实现
    ${IMGUI_SRC}               # <--- 将 ImGui 源码加入项目
)

# 5. 生成可执行文件
add_executable(app ${SRC_LIST})

# 6. 链接库
target_link_libraries(app
    glfw
    OpenGL::GL
    TBB::tbb
    ${CMAKE_DL_LIBS}
    Threads::Threads
)

# 7. 拷贝 Shader
file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/shaders DESTINATION ${CMAKE_CURRENT_BINARY_DIR})

# cmake_minimum_required(VERSION 3.10)
# set(OpenGL_GL_PREFERENCE GLVND)
# project(PointCloud)

# # 1. 强制 C++17 保证对齐和现代语法支持
# set(CMAKE_CXX_STANDARD 17)
# set(CMAKE_CXX_STANDARD_REQUIRED ON)

# # 2. 查找库
# find_package(glfw3 REQUIRED)
# find_package(OpenGL REQUIRED)

# # --- [修改 1/2]: 查找系统线程库 (pthread) ---
# # 2.1 查找多线程支持
# find_package(Threads REQUIRED)

# # 3. 设置 Include 路径
# include_directories(
#     include
#     external/glad/include
# )

# # 4. 自动收集所有源码 (方便后期增加类文件)
# file(GLOB_RECURSE SOURCES "src/*.cpp" "external/glad/src/glad.c")

# # 5. 生成可执行文件
# # add_executable(app src/main.cpp 
# # external/glad/src/glad.c)
# file(GLOB_RECURSE SOURCES "src/*.cpp")
# add_executable(app ${SOURCES})

# # 6. 链接库 (保留你的 dl 库，Linux 下某些环境需要)
# target_link_libraries(app
#     glfw
#     OpenGL::GL
#     ${CMAKE_DL_LIBS}
#     # --- [修改 2/2]: 链接线程库 ---
#     # 6.1 解决 undefined reference to `pthread_create`
#     Threads::Threads
# )

# # 7. 🚀 工业级关键：自动拷贝 Shader 到 Build 目录
# # 这样你在代码里可以直接 fopen("shaders/xxx.vert")
# file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/shaders DESTINATION ${CMAKE_CURRENT_BINARY_DIR})