#pragma once

/**
 * @file Driver.hxx
 * @brief LOCA time-integration driver tying RELAP5 thermal hydraulics to the
 *        fission-gas-release microstructure model.
 *
 * Model geometry
 * --------------
 * A fuel rod is treated as a set of independent axial slices, each of which is
 * a 1D radial stack of MicrostructureCells.  Every slice is integrated on its
 * own using that slice's time-dependent RELAP5 temperature column — i.e.
 * tfuel_surface_K[k * n_axial + (ax_slice - 1)] for slice `ax_slice` — rather
 * than a rod-averaged temperature.  The per-slice step increments are then
 * aggregated to a rod-level FGR, mass-weighted by each slice's releasable gas
 * content (gas_molar_density * hbs_fraction, summed over its radial cells).
 *
 * Per slide 5 of T73 the fuel temperature is uniform within a slice and equal
 * to that slice's external cladding temperature.  For every RELAP5 step
 * k = 1..N-1 the driver:
 *
 *   1. takes the step width dt = time_s[k] - time_s[k-1] (skipping steps where
 *      dt <= 0 — RELAP5 can re-order rows on re-init);
 *   2. per axial slice, computes the pore-gas pressure with the
 *      Van Brutzel-Castelier EOS from that slice's cladding temperature and
 *      calls the FGR model's computeIncrement(dt, T_begin, T_end) once per
 *      radial cell;
 *   3. forms the HBS-weighted radial mean per slice; and
 *   4. aggregates the slices into a rod-level increment using the slice gas
 *      weights, accumulating a running rod-level FGR.
 *
 * Only MicrostructureCells whose fa_number matches opts.fa_number are used;
 * cells belonging to any other FA are ignored.  All cells sharing an ax_slice
 * value form that slice.
 *
 * Errors follow the repository convention: message to std::cout, then
 * exit(1).
 */

#include "tfgr/BaseTFGRModel.hxx"
#include "tfgr/Microstructure.hxx"
#include "tfgr/RelapPlot.hxx"

#include <string>
#include <vector>

namespace tfgr {

struct DriverOptions {
    std::string fa_number;             // e.g. "_00415"
};

struct DriverResult {
    std::vector<double> time_s;
    std::vector<double> T_clad_K;      // mass-weighted cladding T per step
    std::vector<double> P_gap_MPa;     // mass-weighted gap pressure per step
    std::vector<double> tFGR_step;     // rod-level incremental FGR at this step, [-]
    std::vector<double> FGR_cumulative;// running rod-level sum, [-]
    double      FGR_final{};           // == FGR_cumulative.back()
};

/**
 * Run the LOCA transient driver and write one CSV (with header) to @p out_csv.
 *
 * The rod is rebuilt from @p cells by grouping every cell with
 * cell.fa_number == opts.fa_number by its ax_slice.  Each resulting axial
 * slice is integrated independently against its own RELAP5 temperature
 * column; the slices are then mass-weighted into the rod-level result.
 *
 * @param relap     Loaded RELAP5 transient (already parsed by parseRelapPlot).
 * @param cells     MicrostructureCells built from FR + rad_FG tables.
 *                  Only cells matching opts.fa_number are used.
 * @param model     The FGR increment model (currently only DelauneyModel,
 *                  which is a stub returning 0.0). Driver treats its result
 *                  as the per-cell FGR increment; signature unchanged.
 * @param opts      Selects which FA to simulate (all of its axial slices).
 * @param out_csv   Output CSV path. Header is written by the driver.
 * @return          The full DriverResult (also reflected in the CSV).
 */
DriverResult runLocaDriver(const RelapTransient& relap,
                           const std::vector<MicrostructureCell>& cells,
                           const BaseTFGRModel& model,
                           const DriverOptions& opts,
                           const std::string& out_csv);

}  // namespace tfgr
