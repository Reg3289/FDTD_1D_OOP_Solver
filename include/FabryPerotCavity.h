#pragma once
#include "Device.h"
#include "BraggGrating.h" // 拿光栅当做积木来用

class FabryPerotCavity : public Device {
private:
    BraggGrating left_mirror;  // 左反光镜
    BraggGrating right_mirror; // 右反光镜

public:
    // 构造函数极其优雅：我们只需要定义起点、单侧镜子长度、中间腔长
    FabryPerotCavity(int start_idx, int mirror_length, int cavity_length, int period_thickness, double n_high, double n_low)
        : left_mirror(start_idx, start_idx + mirror_length, period_thickness, n_high, n_low),
        right_mirror(start_idx + mirror_length + cavity_length, start_idx + 2 * mirror_length + cavity_length, period_thickness, n_high, n_low) {
    }

    // FP 腔刻画自己的方式，就是命令它的两面镜子分别去刻画自己
    void buildOnGrid(Grid1D& grid) override {
        left_mirror.buildOnGrid(grid);
        right_mirror.buildOnGrid(grid);
    }
};