#pragma once
#include "Grid1D.h"
#include <cmath>

// ==========================================
// 基础源 (绝不能删，它是多态的基础)
// ==========================================
class Source {
public:
    int position;
    Source(int pos) : position(pos) {}
    virtual ~Source() {}
    virtual void injectH(Grid1D& grid, int qTime) = 0;
    virtual void injectE(Grid1D& grid, int qTime) = 0;
};

class UnidirectionalSource : public Source {
public:
    UnidirectionalSource(int pos) : Source(pos) {}
    double getPulse(int qTime) {
        double arg = (qTime + 1.0) - 40.0;
        return std::exp(-std::pow(arg, 2) / 25.0);
    }
    void injectH(Grid1D& grid, int qTime) override {
        grid.hy[position] -= getPulse(qTime) / grid.IMP0;
    }
    void injectE(Grid1D& grid, int qTime) override {
        grid.ez[position] += getPulse(qTime);
    }
};

// ==========================================
// PML 升级版 TF/SF 源
// ==========================================
class TFSFSource : public Source {
public:
    double delay;
    double width;

    TFSFSource(int pos) : Source(pos), delay(40.0), width(5.0) {}

    double getE_inc(double time_step) {
        double arg = time_step - delay;
        return std::exp(-std::pow(arg, 2) / (width * width));
    }

    void injectH(Grid1D& grid, int qTime) override {
        // 利用对应位置的预计算 ch_b 系数
        grid.hy[position - 1] -= getE_inc(qTime) * grid.ch_b[position - 1];
    }

    void injectE(Grid1D& grid, int qTime) override {
        double H_inc = getE_inc(qTime + 1.0) / grid.IMP0;
        // 利用对应位置的预计算 ce_b 系数
        grid.ez[position] += H_inc * grid.ce_b[position];
    }
};