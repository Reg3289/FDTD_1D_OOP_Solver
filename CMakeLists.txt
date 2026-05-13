# 指定 CMake 的最低版本要求
cmake_minimum_required(VERSION 3.15)

# 定义项目名称和使用的语言
project(FDTD_Solver VERSION 1.0 LANGUAGES CXX)

# 设置 C++ 标准为 C++17
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF) # 禁用特定编译器的扩展，保证跨平台性

# 指定头文件的搜索路径
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)

# ==========================================
# 【新增】寻找并引入 OpenMP 库
# ==========================================
find_package(OpenMP REQUIRED)

# 添加可执行文件目标，并将 src/main.cpp 作为源码编译
add_executable(FDTD_1D_OOP_Solver src/main.cpp "include/Device.h")

# ==========================================
# 【新增】将 OpenMP 链接到我们的可执行文件
# ==========================================
if(OpenMP_CXX_FOUND)
    target_link_libraries(FDTD_1D_OOP_Solver PUBLIC OpenMP::OpenMP_CXX)
endif()