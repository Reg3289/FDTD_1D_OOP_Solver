#pragma once
#include "Device.h"

class SingleInterface : public Device {
private:
    int interface_idx;
    double n_left;
    double n_right;

public:
    SingleInterface(int interface_position, double left_index, double right_index)
        : interface_idx(interface_position), n_left(left_index), n_right(right_index) {
    }

    void buildOnGrid(Grid1D& grid) override {
        double eps_left = n_left * n_left;
        double eps_right = n_right * n_right;

        for (int mm = 0; mm < grid.size; ++mm) {
            if (mm < interface_idx) {
                grid.eps_r[mm] = eps_left;
            }
            else {
                grid.eps_r[mm] = eps_right;
            }
        }
    }
};
