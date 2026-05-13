#pragma once
#include "Grid1D.h"
#include <cmath>

class PML {
private:
    int thickness;
    double m_order;
    double R_err;

public:
    // 默认 PML 厚度为 200 层网格，多项式阶数为 3，目标反射误差极其严苛：百万分之一 (1e-6)
    PML(int thick) : thickness(thick), m_order(3.0), R_err(1e-6) {}

    void applyToGrid(Grid1D& grid) {
        // 1. 先初始化全网格的基础系数 (真空中或无损耗器件内部)
        for (int mm = 0; mm < grid.size; ++mm) {
            grid.ce_a[mm] = 1.0;
            grid.ce_b[mm] = grid.IMP0 / grid.eps_r[mm];
        }
        for (int mm = 0; mm < grid.size - 1; ++mm) {
            grid.ch_a[mm] = 1.0;
            grid.ch_b[mm] = 1.0 / grid.IMP0;
        }

        // 2. PML 核心魔法：计算最大衰减因子
        double gamma_max = -(m_order + 1.0) * std::log(R_err) / (4.0 * thickness);

        // 3. 铺设左侧 PML 吸收海绵 (0 到 thickness)
        for (int mm = 0; mm < thickness; ++mm) {
            double depth = thickness - mm; // 越往左边距内部越深，衰减越强
            double gamma = gamma_max * std::pow(depth / thickness, m_order);
            grid.ce_a[mm] = (1.0 - gamma) / (1.0 + gamma);
            grid.ce_b[mm] = (grid.IMP0 / grid.eps_r[mm]) / (1.0 + gamma);

            if (mm < thickness - 1) {
                double depth_h = thickness - mm - 0.5;
                double gamma_h = gamma_max * std::pow(depth_h / thickness, m_order);
                grid.ch_a[mm] = (1.0 - gamma_h) / (1.0 + gamma_h);
                grid.ch_b[mm] = (1.0 / grid.IMP0) / (1.0 + gamma_h);
            }
        }

        // 4. 铺设右侧 PML 吸收海绵
        for (int mm = grid.size - thickness; mm < grid.size; ++mm) {
            double depth = mm - (grid.size - thickness) + 1.0;
            double gamma = gamma_max * std::pow(depth / thickness, m_order);
            grid.ce_a[mm] = (1.0 - gamma) / (1.0 + gamma);
            grid.ce_b[mm] = (grid.IMP0 / grid.eps_r[mm]) / (1.0 + gamma);
        }
        for (int mm = grid.size - thickness; mm < grid.size - 1; ++mm) {
            double depth_h = mm - (grid.size - thickness) + 1.5;
            double gamma_h = gamma_max * std::pow(depth_h / thickness, m_order);
            grid.ch_a[mm] = (1.0 - gamma_h) / (1.0 + gamma_h);
            grid.ch_b[mm] = (1.0 / grid.IMP0) / (1.0 + gamma_h);
        }
    }
};