/**
 * @file Main.cxx
 * @brief 
 *
 * Usage:
 *   tfgr history.csv config.txt output.csv
 */

#include "tfgr/BaseTFGRModel.hxx"
#include "tfgr/DelauneyModel.hxx"
#include "tfgr/InputReader.hxx"
//#include "tfgr/NNModel.hxx"
#include "tfgr/ResultsWriter.hxx"

#include <cstdio>
#include <iostream>
#include <memory>

int main(int argc, char* argv[])
{
    if (argc != 4) {
        std::cerr << "Usage: " << argv[0]
                  << " history.csv config.txt output.csv \n";
        return EXIT_FAILURE;
    }

    const std::string csv_file    = argv[1];
    const std::string config_file = argv[2];
    const std::string output_file = argv[3];

    auto cfg = Config{};
    auto phistory = std::vector<DataPoint>{};

    cfg = parseConfig(config_file);
    phistory = parseCSV(csv_file);

    std::cout << "=== Configuration ===\n"
              << "  model    : " << cfg.model_name << "\n";
    std::cout << "  porosity : " << cfg.porosity  << "\n"
              << "  radius   : " << cfg.radius_um << " µm\n";

    std::unique_ptr<BaseTFGRModel> model;

    if (cfg.model_name == "Delauney")
        model = std::make_unique<DelauneyModel>(cfg.porosity, cfg.radius_um);
    else {
        std::cout << "Unrecognized option" << cfg.model_name << std::endl;
        return 1;
    }

    ResultsWriter writer(output_file);
    writer.writeRow(phistory[0].time_s, phistory[0].temperature_K, 0.0);

    auto FGR = double{0.0};

    std::printf("=== Time integration ===\n");
    std::printf("  step | %11s | %12s | %11s | %12s | %12s\n",
                "time (s)", "T_begin (K)", "T_end (K)", "dFGR", "FGR (/)");
    std::printf("-------|-------------|--------------|-------------|"
                "--------------|-------------\n");

    for (std::size_t i = 1; i < phistory.size(); ++i) {
        const double dt      = phistory[i].time_s - phistory[i-1].time_s;
        const double T_begin = phistory[i-1].temperature_K;
        const double T_end   = phistory[i].temperature_K;

        auto dFGR = double{0.0};
        try {
            dFGR = model->computeIncrement(dt, T_begin, T_end);
        } catch (const std::exception& e) {
            std::cerr << "[ERROR] step " << i << ": " << e.what() << "\n";
            return EXIT_FAILURE;
        }

        FGR += dFGR;

        std::printf("  %5zu | %11.2f | %12.2f | %11.2f | %12.6e | %12.6e\n",
                    i, phistory[i].time_s, T_begin, T_end, dFGR, FGR);

        writer.writeRow(phistory[i].time_s,
                        phistory[i].temperature_K,
                        FGR);
    }

    std::printf("\nDone.  Final FGR = %.6e\n", FGR);
    std::cout << "Results written to: " << output_file << "\n";
    return EXIT_SUCCESS;
}
