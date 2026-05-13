#pragma once
#include "Grid1D.h"
#include "Source.h"
#include <iostream>
#include <fstream>
#include <string>
#include <vector>  // 【新增】为了使用内存缓冲区
#include <chrono>
#include <omp.h>

class Simulation {
private:
    Grid1D& grid;
    int max_time;
    Source* source;
    int probe1_idx;
    int probe2_idx;

public:
    Simulation(Grid1D& g, int t) : grid(g), max_time(t), source(nullptr), probe1_idx(-1), probe2_idx(-1) {}

    void setSource(Source* s) { source = s; }
    void setProbes(int p1, int p2) { probe1_idx = p1; probe2_idx = p2; }

    void run(const std::string& filename) {
        std::cout << "Starting Ultra-Optimized OOP FDTD Engine..." << std::endl;

        auto start_time = std::chrono::high_resolution_clock::now();

        std::ofstream outFile(filename);
        outFile << "TimeStep,Probe1_Ez,Probe2_Ez\n";

        // ==========================================
        // 【核心优化 1】定义固定大小的 I/O 缓冲区 (比如 5000 步)
        // 这些 vector 是局部变量，run 函数结束时内存会自动释放！
        // ==========================================
        const int BUFFER_SIZE = 5000;
        std::vector<int> buf_time(BUFFER_SIZE);
        std::vector<double> buf_probe1(BUFFER_SIZE);
        std::vector<double> buf_probe2(BUFFER_SIZE);
        int buf_idx = 0;

        // ==========================================
        // 【核心优化 2】Auto Shutoff 能量收敛监控变量
        // ==========================================
        double max_energy = 0.0;
        double current_energy = 0.0;
        double shutoff_threshold = 1e-5; // 能量衰减到巅峰的十万分之一时停止
        bool should_stop = false;        // 用于通知所有并行线程退出循环的标志

        #pragma omp parallel 
        {
            for (int qTime = 0; qTime < max_time; ++qTime) {

                // 【注意】如果触发了 Auto Shutoff，所有线程在这里集体跳出循环
                if (should_stop) break;

                // ------------------------------------------
                // 1. 磁场更新 (多线程并行计算)
                // ------------------------------------------
                #pragma omp for simd schedule(static)
                for (int mm = 0; mm < grid.size - 1; ++mm) {
                    grid.hy[mm] = grid.ch_a[mm] * grid.hy[mm] + grid.ch_b[mm] * (grid.ez[mm + 1] - grid.ez[mm]);
                }

                // 磁场源注入 (仅主线程执行，0协商开销)
                 #pragma omp master
                {
                    if (source) source->injectH(grid, qTime);
                }
                #pragma omp barrier // 必须等待 H 场全部更新完毕

                // ------------------------------------------
                // 2. 电场更新 (多线程并行计算)
                // ------------------------------------------
                #pragma omp for simd schedule(static)
                for (int mm = 1; mm < grid.size - 1; ++mm) {
                    grid.ez[mm] = grid.ce_a[mm] * grid.ez[mm] + grid.ce_b[mm] * (grid.hy[mm] - grid.hy[mm - 1]);
                }

                // 电场源注入、数据缓冲与能量监控 (仅主线程执行)
                #pragma omp master
                {
                    if (source) source->injectE(grid, qTime);

                    // --- I/O 缓冲逻辑 ---
                    if (probe1_idx >= 0 && probe2_idx >= 0) {
                        buf_time[buf_idx] = qTime;
                        buf_probe1[buf_idx] = grid.ez[probe1_idx];
                        buf_probe2[buf_idx] = grid.ez[probe2_idx];
                        buf_idx++;

                        // 当缓冲区满了，执行一次集中写硬盘操作
                        if (buf_idx >= BUFFER_SIZE) {
                            for (int i = 0; i < BUFFER_SIZE; ++i) {
                                outFile << buf_time[i] << "," << buf_probe1[i] << "," << buf_probe2[i] << "\n";
                            }
                            buf_idx = 0; // 游标归零，开始覆盖写新的内存，不增加额外内存开销
                        }
                    }

                    if (qTime % 5000 == 0) std::cout << "Step: " << qTime << " / " << max_time << std::endl;

                    // --- Auto Shutoff 能量收敛监控逻辑 (每 100 步检查一次，省 CPU) ---
                    if (qTime % 100 == 0) {
                        current_energy = 0.0;
                        // 粗略计算当前系统总电场能量
                        for (int i = 0; i < grid.size; ++i) {
                            current_energy += grid.eps_r[i] * grid.ez[i] * grid.ez[i];
                        }

                        if (current_energy > max_energy) {
                            max_energy = current_energy; // 记录历史最高能量
                        }

                        // 如果脉冲已经打完，且当前能量已经微不足道，触发停止！
                        if (max_energy > 1e-10 && (current_energy / max_energy) < shutoff_threshold) {
                            std::cout << ">>> Auto Shutoff Triggered at step " << qTime << " ! Energy has decayed to target level. <<<" << std::endl;
                            should_stop = true; // 改变标志位，让其他线程在下一回合退出
                        }
                    }
                }
            #pragma omp barrier // 必须等待 E场更新和主线程业务完毕，再进入下一时间步
            }
        }

        // ==========================================
        // 【核心优化 3】善后工作：将缓冲区里最后剩下的、没凑够 5000 步的尾巴数据写进硬盘
        // ==========================================
        if (buf_idx > 0) {
            for (int i = 0; i < buf_idx; ++i) {
                outFile << buf_time[i] << "," << buf_probe1[i] << "," << buf_probe2[i] << "\n";
            }
        }

        outFile.close(); // 关闭文件

        auto end_time = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double> elapsed = end_time - start_time;
        std::cout << "Simulation Complete! Total Execution Time: " << elapsed.count() << " seconds." << std::endl;

        // 函数在这里结束，buf_time, buf_probe1, buf_probe2 的内存会被 C++ 自动回收！
    }
};


/*
    void run(const std::string& filename) {
        std::ofstream outFile(filename);
        std::cout << "Starting OOP FDTD Engine with PML & OpenMP..." << std::endl;
        outFile << "TimeStep,Probe1_Ez,Probe2_Ez\n";

        auto start_time = std::chrono::high_resolution_clock::now();



        //只是采用多线程实现  由于每次使用多线程都会有一个 打开关闭的操作  1.21275 seconds
        //OpenMP 的底层运行机制叫 Fork - Join 模型。当遇到这行指令时，主线程会把其他沉睡的线程“唤醒（Fork）”去干活；
        //  干完活后，又要花费时间把它们“收拢并挂起（Join）”。
        //  线程管理的开销甚至会大于计算本身的开销。
        for (int qTime = 0; qTime < max_time; ++qTime) {
            // ==========================================
            // 【核心升级】由预计算的系数矩阵驱动磁场更新
            // ==========================================
            #pragma omp parallel for
            for (int mm = 0; mm < grid.size - 1; ++mm) {
                grid.hy[mm] = grid.ch_a[mm] * grid.hy[mm] + grid.ch_b[mm] * (grid.ez[mm + 1] - grid.ez[mm]);
            }

            if (source) source->injectH(grid, qTime);

            // ==========================================
            // 【核心升级】由预计算的系数矩阵驱动电场更新
            // ==========================================
            #pragma omp parallel for
            for (int mm = 1; mm < grid.size - 1; ++mm) {
                grid.ez[mm] = grid.ce_a[mm] * grid.ez[mm] + grid.ce_b[mm] * (grid.hy[mm] - grid.hy[mm - 1]);
            }

            if (source) source->injectE(grid, qTime);

            // 注意：这里删除了原来累赘的 boundary->apply() 代码！

            if (probe1_idx >= 0 && probe2_idx >= 0) {
                outFile << qTime << "," << grid.ez[probe1_idx] << "," << grid.ez[probe2_idx] << "\n";
            }
            if (qTime % 1000 == 0) std::cout << "Step: " << qTime << " / " << max_time << std::endl;
        }

    */