#pragma once
#include "Device.h" // 引入基类

// 继承自 Device
class BraggGrating : public Device {
private:
    int start_idx;
    int end_idx;
    int period_thickness;
    double n_high;
    double n_low;

public:
    BraggGrating(int start, int end, int thickness, double nh, double nl)
        : start_idx(start), end_idx(end), period_thickness(thickness), n_high(nh), n_low(nl) {
    }

    // 加上 override 明确重写基类虚函数
    void buildOnGrid(Grid1D& grid) override {
        double eps_high = n_high * n_high;
        double eps_low = n_low * n_low;
        int period_total = period_thickness * 2;

        for (int mm = start_idx; mm < end_idx; ++mm) {
            if ((mm - start_idx) % period_total < period_thickness) {
                grid.eps_r[mm] = eps_high;
            }
            else {
                grid.eps_r[mm] = eps_low;
            }
        }
    }
};