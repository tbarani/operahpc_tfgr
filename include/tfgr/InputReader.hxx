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
