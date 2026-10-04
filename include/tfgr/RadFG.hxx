#pragma once

/**
 * @file RadFG.hxx
 * @brief Parser for radial fission gas release (*_rad_FG_*.dat) files.
 */

#include <string>
#include <vector>

namespace tfgr {

/**
 * One radial node record from a *_rad_FG_*.dat file.
 *
 * The source file has two header lines (column names + units), then
 * whitespace-separated data rows grouped per (FA_number, Ax_slice).
 */
struct RadFGRow {
    std::string fa_number;        // e.g. "_00415"
    std::string core_pos;         // e.g. "2,1"
    int         ax_slice{};       // 1-based axial slice index
    int         rad_node{};       // 1-based radial node index (1..23)
    double      rel_rad{};        // relative radial position, [-], 0..1
    double      rad_burn{};       // radial burnup, MWd/tM
    double      rad_gas_c{};      // created fission gas, umol/mm^3
    double      rad_gas_s{};      // stored fission gas, umol/mm^3
};

/**
 * Parse a *_rad_FG_*.dat file.
 *
 * - The first two non-blank lines are treated as headers and skipped.
 * - Blank lines (used to separate groups) are skipped.
 * - CRLF line endings are tolerated.
 * - Every data row must contain exactly 8 tokens, otherwise the program
 *   prints an error to std::cout and exits with status 1.
 * - A Rel_rad value outside [0, 1] triggers a warning but is not fatal.
 *
 * @throws Does not throw; malformed input terminates the process.
 */
std::vector<RadFGRow> parseRadFG(const std::string& filename);

}  // namespace tfgr
