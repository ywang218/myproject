#!/usr/bin/env bash
# 一键编译并运行 PointCloud 仿真程序
# 用法: ./run.sh           (有改动会自动重新编译，然后运行)
#       ./run.sh rebuild   (强制清空重新编译)
set -e
cd "$(dirname "$0")"

# WSL 默认 OpenGL 只有 4.2，本项目需要 4.3（compute shader）。
# 用 Zink 让 OpenGL 跑在 Vulkan 上，拿到 GL 4.6。
export MESA_LOADER_DRIVER_OVERRIDE=zink

if [ "$1" = "rebuild" ]; then
    rm -rf build
fi

mkdir -p build
cd build
cmake -DCMAKE_BUILD_TYPE=Release .. >/dev/null
make -j"$(nproc)"

echo "=== 启动 app (Zink/OpenGL 4.6) ==="
exec ./app
