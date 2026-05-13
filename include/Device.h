#pragma once
#include "Grid1D.h"

class Device {
public:
    virtual ~Device() {}
    // 纯虚函数：强制所有子类必须实现这个方法
    virtual void buildOnGrid(Grid1D& grid) = 0;
};