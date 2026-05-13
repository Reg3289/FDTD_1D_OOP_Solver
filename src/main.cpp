#include "../include/Grid1D.h"
#include "../include/Boundary.h"
#include "../include/Source.h"
#include "../include/BraggGrating.h"
#include "../include/FabryPerotCavity.h"
#include "../include/SingleInterface.h"
#include "../include/Simulation.h"

#include <iostream>
#include <omp.h>
#include <string>

int main(int argc, char* argv[]) {
    std::string mode = "device";

    if (argc > 1) {
        mode = argv[1];
    }

    std::string output_file;

    Grid1D grid(100000);

    if (mode == "reference") {
        output_file = "fdtd_reference.csv";
        std::cout << "Running reference simulation without device..." << std::endl;
    }
    else if (mode == "interface") {
        output_file = "fdtd_interface.csv";
        std::cout << "Running single-interface Fresnel validation simulation..." << std::endl;

        SingleInterface interface_device(600, 1.0, 1.5);
        interface_device.buildOnGrid(grid);
    }
    else if (mode == "device") {
        output_file = "fdtd_device.csv";
        std::cout << "Running device simulation with Fabry-Perot cavity..." << std::endl;

        FabryPerotCavity fp_cavity(400, 70, 30, 7, 2.0, 1.5);
        fp_cavity.buildOnGrid(grid);
    }
    else {
        std::cerr << "Unknown simulation mode: " << mode << std::endl;
        std::cerr << "Usage:" << std::endl;
        std::cerr << "  ./FDTD_1D_OOP_Solver reference" << std::endl;
        std::cerr << "  ./FDTD_1D_OOP_Solver interface" << std::endl;
        std::cerr << "  ./FDTD_1D_OOP_Solver device" << std::endl;
        return 1;
    }

    // Apply PML after the material distribution has been written to the grid.
    PML myPML(200);
    myPML.applyToGrid(grid);

    TFSFSource mySource(300);

    Simulation engine(grid, 50000);
    engine.setSource(&mySource);

    // Probe 1 is placed in the scattered-field region.
    // Probe 2 is placed on the transmitted side.
    engine.setProbes(250, 700);

    omp_set_num_threads(omp_get_max_threads());

    engine.run(output_file);

    std::cout << "Output file: " << output_file << std::endl;

    return 0;
}
