/**
 * @file Main.cxx
 * @brief tfgr entry point.
 *
 * Usage:
 *   tfgr history.csv config.txt output.csv
 *
 * Config-driven modes:
 *   - "Legacy" CSV flow (default): config.txt has model/porosity/radius;
 *     history.csv is a (time_s, temperature_K) trajectory that gets
 *     integrated against the chosen FGR model.
 *   - "LOCA driver" flow (Task 7.3): if config.txt additionally sets
 *     fa_number, fr_input, rad_fg, relap_plot, the executable loads the
 *     full GALILEE + RELAP5 dataset and runs the multi-slice
 *     runLocaDriver flow that aggregates per-slice rod-level releases.
 *     The history.csv argument is ignored in that mode.
 */

#include "tfgr/BaseTFGRModel.hxx"
#include "tfgr/DelauneyModel.hxx"
#include "tfgr/Driver.hxx"
#include "tfgr/Eos.hxx"
#include "tfgr/FRInput.hxx"
#include "tfgr/InputReader.hxx"
#include "tfgr/Microstructure.hxx"
#include "tfgr/NNModel.hxx"
#include "tfgr/RadFG.hxx"
#include "tfgr/RelapPlot.hxx"
#include "tfgr/ResultsWriter.hxx"
#include "tfgr/Validator.hxx"

#include <cstdio>
#include <iostream>
#include <memory>
#include <string>

namespace {

// True iff every LOCA-driver key is present (non-empty) in @p cfg.
bool hasLocaDriverKeys(const Config& cfg)
{
    return !cfg.fa_number.empty()
        && !cfg.fr_input.empty()
        && !cfg.rad_fg.empty()
        && !cfg.relap_plot.empty();
}

// Legacy CSV-driven flow (the original Main.cxx).
int runLegacyCsvFlow(const std::string& csv_file,
                     const Config& cfg,
                     const std::string& output_file)
{
    auto phistory = parseCSV(csv_file);

    std::cout << "=== Configuration ===\n"
              << "  model    : " << cfg.model_name << "\n";
    std::cout << "  porosity : " << cfg.porosity  << "\n"
              << "  radius   : " << cfg.radius_um << " µm\n";

    std::unique_ptr<BaseTFGRModel> model;
    if (cfg.model_name == "Delauney") {
        model = std::make_unique<DelauneyModel>(cfg.porosity, cfg.radius_um);
    } else if (cfg.model_name == "NN") {
        model = std::make_unique<tfgr::NNModel>(cfg.nn_model_path);
    } else {
        std::cout << "Unrecognized option " << cfg.model_name << std::endl;
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

}  // namespace

int main(int argc, char* argv[])
{
    if (argc != 4 && argc != 5) {
        std::cerr << "Usage: " << argv[0]
                  << " history.csv config.txt output.csv [ref_csv]\n"
                  << "  ref_csv (optional): Jernkvist-2019 reference FGR CSV\n"
                  << "                     for validation after a LOCA run.\n";
        return EXIT_FAILURE;
    }

    // The first positional is named "history.csv" by the usage message, but
    // in the LOCA-driver flow it is unused.  We parse it for compatibility
    // (and to fail fast if it's missing); the LOCA flow reads its real
    // inputs from the config keys.
    const std::string history_csv = argv[1];
    const std::string config_file = argv[2];
    const std::string output_file = argv[3];
    const std::string ref_csv     = (argc == 5) ? argv[4] : std::string{};

    const Config cfg = parseConfig(config_file);

    if (hasLocaDriverKeys(cfg)) {
        // ---- LOCA-driver flow (Task 7.3) ----
        std::cout << "=== LOCA-driver mode ===\n"
                  << "  FA         : " << cfg.fa_number << "\n"
                  << "  FR input   : " << cfg.fr_input << "\n"
                  << "  rad FG     : " << cfg.rad_fg << "\n"
                  << "  RELAP plot : " << cfg.relap_plot << "\n"
                  << "  model      : " << cfg.model_name << "\n";

        const auto fr_rows = tfgr::parseFRInput(cfg.fr_input);
        const auto fg_rows = tfgr::parseRadFG(cfg.rad_fg);
        const auto relap   = tfgr::parseRelapPlot(cfg.relap_plot);

        const auto cells = tfgr::buildMicrostructures(fr_rows, fg_rows);

        std::cout << "  FR rows    : " << fr_rows.size() << "\n"
                  << "  rad FG rows: " << fg_rows.size() << "\n"
                  << "  RELAP steps: " << relap.time_run.size() << "\n"
                  << "  cells (all): " << cells.size() << "\n";

        std::unique_ptr<BaseTFGRModel> model;
        if (cfg.model_name == "Delauney") {
            // The driver does not consume porosity_/radius_um from the
            // model — each MicrostructureCell carries its own.  We pass
            // cfg.porosity / cfg.radius_um only because the existing
            // constructor requires them.
            model = std::make_unique<DelauneyModel>(cfg.porosity,
                                                    cfg.radius_um);
        } else if (cfg.model_name == "NN") {
            model = std::make_unique<tfgr::NNModel>(cfg.nn_model_path);
        } else {
            std::cout << "Unrecognized model option " << cfg.model_name
                      << " for LOCA-driver mode.\n";
            return EXIT_FAILURE;
        }

        const tfgr::DriverOptions opts{cfg.fa_number};
        const auto drv = tfgr::runLocaDriver(relap, cells, *model, opts,
                                             output_file);

        std::cout << "  driver rows: " << drv.time_s.size() << "\n";
        std::printf("\nDone.  Final rod-level FGR = %.6e\n", drv.FGR_final);
        std::cout << "Results written to: " << output_file << "\n";

        if (!ref_csv.empty()) {
            // Phase E validation: diff against a Jernkvist-2019 reference.
            // Threshold = 0.10 (10 percentage points) — generous placeholder
            // until the MMM database lands and our predicted FGR stabilises.
            constexpr double kThreshold = 0.10;
            const auto vr = tfgr::validateAgainstJerkvist(
                output_file, ref_csv, kThreshold);

            std::printf("\n=== Validation vs %s ===\n", ref_csv.c_str());
            std::printf("  our rows       : %zu\n", vr.n_ours);
            std::printf("  ref rows       : %zu\n", vr.n_ref);
            std::printf("  max_abs_error  : %.6e\n", vr.max_abs_error);
            std::printf("  mean_abs_error : %.6e\n", vr.mean_abs_error);
            std::printf("  final_ours     : %.6e\n", vr.final_ours);
            std::printf("  final_ref      : %.6e\n", vr.final_ref);
            std::printf("  threshold      : %.6e\n", kThreshold);
            std::printf("  passed         : %s\n",
                        vr.passed ? "yes" : "no");

            if (!vr.passed) {
                return EXIT_FAILURE;
            }
        }
        return EXIT_SUCCESS;
    }

    // ---- Legacy CSV flow ----
    (void)history_csv;  // silence the "unused" warning when in LOCA mode is
                         // impossible (we got here only if hasLocaDriverKeys
                         // returned false).
    return runLegacyCsvFlow(history_csv, cfg, output_file);
}