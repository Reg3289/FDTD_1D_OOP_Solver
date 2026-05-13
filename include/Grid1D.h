#pragma once
#include <vector>

class Grid1D {
public:
    int size;
    double IMP0;
    std::vector<double> ez;
    std::vector<double> hy;
    std::vector<double> eps_r;

    // 【核心新增】工业级 FDTD 的标志：预计算更新系数矩阵
    std::vector<double> ce_a;
    std::vector<double> ce_b;
    std::vector<double> ch_a;
    std::vector<double> ch_b;

    Grid1D(int s) : size(s), IMP0(377.0) {
        ez.assign(size, 0.0);
        hy.assign(size - 1, 0.0);
        eps_r.assign(size, 1.0);

        ce_a.assign(size, 1.0);
        ce_b.assign(size, 1.0);
        ch_a.assign(size - 1, 1.0);
        ch_b.assign(size - 1, 1.0);
    }
};