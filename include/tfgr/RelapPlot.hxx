#pragma once

/**
 * @file RelapPlot.hxx
 * @brief Loader for RELAP5 ".5tm" plot files (VVER LOCA transient results,
 *        e.g. VVER_LOCA_RELAP_Transient_Results/5tm.491).
 *
 * File layout
 * -----------
 * Four header rows, each space-separated with every token wrapped in double
 * quotes, CRLF line endings:
 *
 *   1. type row        : "time" "cntrlvar" (22x "httemp") "cntrlvar"
 *                        (22x "httemp") "cntrlvar" (22x "pgap")
 *   2. identifier row  : "0" "991" (22x "49100NNN") "991" (22x "49100NNN")
 *                        "991" (22x "49100NNN")
 *   3. display-name row: "time_run" "time_s" (22x "Tfuel-0491") "--BALAST--"
 *                        (22x "Tfuel-0491") "--BALAST--" (22x "Pgap--0491")
 *   4. units row       : 70x "si"
 *
 * Header tokens are padded with spaces *inside* the quotes, so a naive
 * whitespace split would break them; this loader extracts quoted substrings
 * instead. The first non-space character of a header line is '"'.
 *
 * Data rows then follow: 70 unquoted whitespace-separated numerics in
 * scientific notation, CRLF terminated. Column layout (0-based):
 *
 *     col  0        : time_run
 *     col  1        : time_s   (transient time; may decrease)
 *     col  2..23    : Tfuel surface, 22 axial nodes, K
 *     col 24        : BALAST sentinel (duplicates time_s)
 *     col 25..46    : Tfuel center, 22 axial nodes, K
 *     col 47        : BALAST sentinel (duplicates time_s)
 *     col 48..69    : Pgap, 22 axial nodes, Pa
 *
 * The two BALAST sentinel columns are skipped. Arrays are indexed
 * [step * n_axial + node], with n_axial = 22 for this format.
 *
 * Pressure units
 * --------------
 * RELAP5 .5tm plot files output `pgap` in Pa; the value is kept as-is and is
 * documented here as Pa. No unit conversion is applied.
 *
 * Note: AGENTS.md mandates errors go to std::cout + exit(1); this loader
 * matches that convention.
 */

#include <cstddef>
#include <string>
#include <vector>

namespace tfgr {

struct RelapTransient {
    std::string assembly_id;                  // e.g. "0491"
    std::vector<double> time_run;             // col 0
    std::vector<double> time_s;               // col 1 (transient time, can decrease)
    // Indexed [step * n_axial + node], 22 nodes per step.
    std::vector<double> tfuel_surface_K;      // col 2..23, K
    std::vector<double> tfuel_center_K;       // col 25..46, K
    std::vector<double> pgap_Pa;              // col 48..69, Pa (RELAP outputs Pa)
    std::size_t         n_axial{};            // 22
};

/**
 * Parse a RELAP5 .5tm plot file.
 *
 * Exits with code 1 (via std::cout + exit(1), matching the repo convention)
 * on a missing file, malformed/missing headers, a data row that does not have
 * exactly 70 columns, or a non-numeric data field.
 */
RelapTransient parseRelapPlot(const std::string& filename);

}  // namespace tfgr
