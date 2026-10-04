#pragma once

/**
 * @file PowerHistory.hxx
 * @brief Loader for the OperaHPC power-history tables
 *        (.../PowerHistories/Axial_Resolution_{10,22}/VVER_Temelin_18M_FA__NNNNN.ph).
 *
 * File layout:
 *   line 1 : "Power history for FA: _00415  Core_pos: 2,1"
 *   line 2 : column names, one triple of columns per axial node
 *              Time Burnup_01 .. Burnup_NN  LHGR_01 .. LHGR_NN  FNFlux_01 .. FNFlux_NN
 *   line 3 : units
 *              [EFPH] [MWd/tM] ... [kW/m] ... [n/(cm**2 s)] ...
 *   data   : <time_efph> <burnup_01..NN> <lhgr_01..NN> <fnflux_01..NN>
 *
 * The axial resolution N (10 or 22) is detected from the header token count,
 * never hardcoded.
 *
 * Note: AGENTS.md says errors go to std::cout + exit(1); match that style.
 */

#include <cstddef>
#include <string>
#include <vector>

namespace tfgr {

struct PowerCurve {
    std::string fa_number;            // "_00415"
    std::string core_pos;             // "2,1"
    std::size_t n_nodes{};            // axial nodes: 10 or 22
    std::vector<double> t_efph;       // time in EFPH, one entry per step
    // The three arrays below are indexed [step * n_nodes + node].
    std::vector<double> burnup;       // MWd/tM
    std::vector<double> lhgr;         // kW/m
    std::vector<double> fnflux;       // n/(cm^2 s)
};

/**
 * Parse a .ph power-history file.
 *
 * Exits with code 1 (via std::cout + exit(1), matching repo convention)
 * on a missing file, malformed header, wrong token count, or non-numeric data.
 */
PowerCurve parsePowerHistory(const std::string& filename);

}  // namespace tfgr
