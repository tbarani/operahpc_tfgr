#pragma once

/**
 * @file FRInput.hxx
 * @brief Loader for the GALILEE fuel-rod FR input table
 *        (VVER_Temelin_18M_Standard-clad_FR_input_LOCA_{BOC,MOC,EOC}_Ax{10,22}.dat).
 *
 * Two header lines (column names, units) followed by data rows of the form:
 *
 *     _00415       2,1         1     19719  0.010191  0.010485     5.243  ...
 *      FA_number Core_pos Ax_slice Ax_burnup  ROB  RFD  PGAS  ...
 *
 * Columns (25 total):
 *   0  FA_number    (string, "_NNNNN")
 *   1  Core_pos     (string, "i,j")
 *   2  Ax_slice     (int,    1..N_axial)
 *   3  Ax_burnup    MWd/tM
 *   4  ROB          g/mm^3
 *   5  RFD          g/mm^3
 *   6  PGAS         MPa
 *   7  XHE          [-]
 *   8  DBA          mm
 *   9  DHI          mm
 *  10  D0           mm
 *  11  DHA          mm
 *  12  OXT          mm
 *  13  POW          W/mm
 *  14  TBI          degC
 *  15  TBA          degC
 *  16  TBM          degC
 *  17  THI          degC
 *  18  THA          degC
 *  19  THM          degC
 *  20  AFTC         W/(m*K)
 *  21  AHTC         W/(m*K)
 *  22  SEFL         kJ/mm
 *  23  SEFM         kJ/g(UO2)
 *  24  SEHL         kJ/mm
 *  25  GAPHTC       W/(K*mm^2)
 *
 * Note: AGENTS.md says errors go to std::cout + exit(1); match that style.
 */

#include <string>
#include <vector>

namespace tfgr {

struct FRRow {
    std::string fa_number;    // "_00415"
    std::string core_pos;     // "2,1"
    int         ax_slice{};   // 1-based axial slice index
    double      ax_burnup{};  // MWd/tM
    double      rob{};        // g/mm^3
    double      rfd{};        // g/mm^3
    double      pgas{};       // MPa
    double      xhe{};        // [-]
    double      dba{};        // mm
    double      dhi{};        // mm
    double      d0{};         // mm
    double      dha{};        // mm
    double      oxt{};        // mm
    double      pow{};        // W/mm
    double      tbi{};        // degC
    double      tba{};        // degC
    double      tbm{};        // degC
    double      thi{};        // degC
    double      tha{};        // degC
    double      thm{};        // degC
    double      aftc{};       // W/(m*K)
    double      ahtc{};       // W/(m*K)
    double      sefl{};       // kJ/mm
    double      sefm{};       // kJ/g(UO2)
    double      sehl{};       // kJ/mm
    double      gaphtc{};     // W/(K*mm^2)
};

/**
 * Parse a GALILEE FR input file.
 *
 * Exits with code 1 (via std::cout + exit(1), matching repo convention)
 * on missing file or malformed rows.
 */
std::vector<FRRow> parseFRInput(const std::string& filename);

}  // namespace tfgr