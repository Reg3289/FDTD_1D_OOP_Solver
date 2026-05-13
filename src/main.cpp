#include "../include/Grid1D.h"
#include "../include/Boundary.h" // 里面装的是 PML
#include "../include/Source.h"
#include "../include/BraggGrating.h"
#include "../include/FabryPerotCavity.h"
#include "../include/Simulation.h"
#include <omp.h>

int main() {
    Grid1D grid(100000);

    Device* myDevice = nullptr;
    FabryPerotCavity fp_cavity(400, 70, 30, 7, 2.0, 1.5);
    myDevice = &fp_cavity;
    if (myDevice) {
        myDevice->buildOnGrid(grid);
    }

    // ==========================================
    // 【核心新增】必须在器件放置完之后，铺设 PML 海绵！
    // 左右各铺设 200 层的完美匹配层
    // ==========================================
    PML myPML(200);
    myPML.applyToGrid(grid);

    TFSFSource mySource(300);

    Simulation engine(grid, 50000);
    // 移除了 engine.setBoundary()
    engine.setSource(&mySource);
    engine.setProbes(250, 700);

    omp_set_num_threads(omp_get_max_threads());
    engine.run("fdtd_probes_results.csv");

    return 0;
}