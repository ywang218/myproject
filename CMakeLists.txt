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

# 1. 强制 C++17
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 2. 查找库
find_package(glfw3 REQUIRED)
find_package(OpenGL REQUIRED)
find_package(Threads REQUIRED)

# 3. 设置 Include 路径
include_directories(
    include
    external/glad/include
)

# 4. 精确指定源码（避免 GLOB 抓取到多余的 main 函数）
# 建议手动列出文件，或者确保 src 目录下只有一个 main.cpp
set(SRC_LIST
    src/main.cpp
    src/renderer/Renderer.cpp
    src/renderer/PointCloudLayer.cpp
    external/glad/src/glad.c  # <--- 关键实现
)

# 5. 生成可执行文件
add_executable(app ${SRC_LIST})

# 6. 链接库
target_link_libraries(app
    glfw
    OpenGL::GL
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