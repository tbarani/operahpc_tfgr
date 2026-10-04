#pragma once

/**
 * @file InputReader.hxx
 * @brief Input file and CSV parsing.
 */

#include <string>
#include <vector>

struct Config {
    std::string model_name;
    double      porosity;
    double      radius_um;
    std::string nn_model_path;

    // LOCA-driver extension (T73): all four are optional.  When all are
    // set Main.cxx uses the multi-slice runLocaDriver flow; otherwise the
    // historical CSV-driven path is used.  `fa_number` is the fuel assembly
    // selector (e.g. "_00415") and the three paths point at the GALILEE /
    // RELAP5 data files for that assembly.
    std::string fa_number;
    std::string fr_input;
    std::string rad_fg;
    std::string relap_plot;
};

/**
 * Parse a key=value settings file.
 *
 * Required keys:  model, porosity, radius
 * Optional key:   nn_model  (mandatory when model=NN)
 * Lines starting with '#' are ignored.
 *
 */
Config parseConfig(const std::string& filename);

struct DataPoint {
    double time_s;
    double temperature_K;
};

/**
 * Read a two-column CSV (time [s], temperature [K]).
 *
 * - Lines starting with '#' are ignored.
 * - An optional header row (non-numeric first field) is skipped.
 * - Rows are sorted chronologically after loading.
 */

std::vector<DataPoint> parseCSV(const std::string& filename);
