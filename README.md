# 1D FDTD OOP Solver

## 项目简介

本项目是一个基于 C++17 的一维 FDTD（Finite-Difference Time-Domain，时域有限差分）电磁仿真程序。代码采用面向对象方式组织，将一维网格、光源、光学器件、PML 吸收边界和仿真主循环分别封装成独立类，便于后续扩展不同器件结构、激励源和仿真策略。

当前示例程序会在一维网格中构建一个 Fabry-Perot 腔结构，使用 TF/SF 高斯脉冲源激励系统，并通过两个探针记录电场随时间变化，最终输出 CSV 结果文件。

## 主要功能

- 一维 FDTD 电磁场更新，包含 `Ez` 电场和 `Hy` 磁场。
- 支持相对介电常数 `eps_r` 空间分布，用于描述不同折射率材料。
- 支持预计算更新系数 `ce_a`、`ce_b`、`ch_a`、`ch_b`。
- 支持 PML 吸收边界，用于降低边界反射。
- 支持抽象光源接口，并提供单向源和 TF/SF 源实现。
- 支持抽象器件接口，并提供 Bragg 光栅和 Fabry-Perot 腔结构。
- 使用 OpenMP 对主场更新循环进行并行化。
- 支持探针采样并将结果缓冲写入 CSV 文件。
- 支持基于场能量衰减的 Auto Shutoff 自动停止逻辑。

## 目录结构

```text
FDTD_1D_OOP_Solver/
├── CMakeLists.txt
├── CMakePresets.json
├── README.md
├── include/
│   ├── Boundary.h
│   ├── BraggGrating.h
│   ├── Device.h
│   ├── FabryPerotCavity.h
│   ├── Grid1D.h
│   ├── Simulation.h
│   └── Source.h
├── src/
│   └── main.cpp
├── out/
│   └── 构建输出目录
└── .vs/
    └── Visual Studio 本地配置与缓存
```

## 编译运行方法

项目使用 CMake 构建，并在 `CMakeLists.txt` 中启用了 C++17 和 OpenMP。

### 使用 CMake Preset

在支持 CMake Presets 的环境中，可以使用以下命令配置和构建：

```powershell
cmake --preset x64-debug
cmake --build out/build/x64-debug
```

运行可执行文件：

```powershell
.\out\build\x64-debug\fdtd_1D_OOP_Solver.exe
```

程序运行后会在当前运行目录生成：

```text
fdtd_probes_results.csv
```

### 手动 CMake 构建

如果不使用 preset，也可以手动指定构建目录：

```powershell
cmake -S . -B out/build/manual
cmake --build out/build/manual
```

然后运行生成的可执行文件。具体路径取决于当前平台、生成器和构建配置。

## 主要类说明

- `Grid1D`：一维仿真网格，保存 `Ez`、`Hy`、`eps_r` 以及 FDTD 更新系数。
- `PML`：吸收边界类，负责把左右两侧 PML 参数写入网格更新系数。
- `Source`：光源抽象基类，定义 `injectH()` 和 `injectE()` 两个源注入接口。
- `UnidirectionalSource`：简单单向高斯脉冲源实现。
- `TFSFSource`：TF/SF 高斯脉冲源实现，当前主程序使用该源。
- `Device`：器件抽象基类，定义 `buildOnGrid()` 接口。
- `BraggGrating`：Bragg 光栅器件，将网格中的 `eps_r` 设置为高低折射率周期结构。
- `FabryPerotCavity`：Fabry-Perot 腔器件，由左右两个 Bragg 光栅反射镜组合而成。
- `Simulation`：仿真引擎，负责主时间循环、场更新、源注入、探针记录、文件输出和自动停止判断。

## 仿真流程简介

当前 `src/main.cpp` 中的仿真流程如下：

1. 创建一维网格 `Grid1D grid(100000)`。
2. 创建 Fabry-Perot 腔，并通过 `buildOnGrid()` 写入材料分布。
3. 创建并应用 PML，使网格两端具备吸收边界。
4. 创建 TF/SF 源，设置源位置。
5. 创建 `Simulation` 仿真引擎，设置最大时间步数。
6. 设置光源和两个探针位置。
7. 设置 OpenMP 线程数。
8. 调用 `engine.run("fdtd_probes_results.csv")` 开始仿真。
9. 在主循环中依次更新 `Hy`、注入磁场源、更新 `Ez`、注入电场源、记录探针数据并检查自动停止条件。
10. 仿真结束后将剩余缓冲数据写入 CSV 文件，并输出运行耗时。

## 后续可优化方向

- 增加参数合法性检查，例如网格大小、PML 厚度、源位置、探针位置和器件区间边界。
- 将 `Simulation::run()` 进一步拆分，分离场更新、I/O、探针采样、停止准则和日志输出职责。
- 将探针系统扩展为可配置的多探针结构，而不是固定两个探针。
- 将 Auto Shutoff 的能量计算改为 OpenMP reduction，降低大网格下的串行开销。
- 增加配置文件或命令行参数，避免在 `main.cpp` 中硬编码网格大小、器件参数和输出文件名。
- 增加基础单元测试或数值回归测试，方便后续重构时验证仿真结果未发生意外变化。
- 改进 CMake 目标配置，例如将头文件全部纳入目标源列表，或使用 `target_include_directories()` 替代全局 `include_directories()`。
