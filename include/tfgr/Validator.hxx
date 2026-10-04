#pragma once

/**
 * @file Validator.hxx
 * @brief Jernkvist (2019) validation: compare a tfgr CSV output against a
 *        reference cumulative-FGR trajectory.
 */

#include <cstddef>
#include <string>
#include <vector>

namespace tfgr {

/// One (time, cumulative FGR) sample read from a tfgr CSV output file.
///
/// Only the time and the cumulative FGR column are retained; the other
/// columns of the 5-column LOCA-driver output are ignored.
struct TimeFGRPair {
    double time_s{};          ///< time                                        [s]
    double fgr_cumulative{};  ///< cumulative fission gas release             [/]
};

/// Outcome of validateAgainstJerkvist().
struct ValidationReport {
    std::size_t n_ours{};       ///< rows in our CSV
    std::size_t n_ref{};        ///< rows in reference CSV
    double max_abs_error{};     ///< max |FGR_ours(t) - FGR_ref(t)|
    double mean_abs_error{};    ///< mean over all matched times
    double final_ours{};        ///< FGR_cumulative[-1] in ours
    double final_ref{};         ///< FGR_cumulative[-1] in reference
    bool   passed{};            ///< max_abs_error <= threshold
};

/**
 * Read a tfgr CSV output file.
 *
 * Expected column layout (LOCA driver, Driver.cxx):
 *   time_s,T_clad_K,P_gap_MPa,tFGR_step,FGR_cumulative
 *
 * Only columns 0 and 4 are read.  Tolerant of CRLF, leading/trailing
 * whitespace, blank lines and '#' comments.  A leading header line (or any
 * line whose first token does not parse as a number) is skipped, as are
 * lines with fewer than five comma-separated tokens.  Rows are returned in
 * file order.
 *
 * If @p path cannot be opened, prints to std::cout and calls exit(1).
 */
std::vector<TimeFGRPair> readFgrCsv(const std::string& path);

/**
 * Run validation against a reference CSV.
 *
 * Reads both CSVs, linearly interpolates the reference at every one of our
 * timestamps (clamping outside the reference range), and computes the max
 * and mean absolute FGR errors plus the final values of each trajectory.
 *
 * @param our_csv    tfgr output CSV to test.
 * @param ref_csv    Reference (Jernkvist 2019) CSV.
 * @param threshold  Pass/fail bound on max_abs_error.
 */
ValidationReport validateAgainstJerkvist(const std::string& our_csv,
                                         const std::string& ref_csv,
                                         double threshold);

}  // namespace tfgr
