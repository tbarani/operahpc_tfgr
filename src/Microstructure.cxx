/**
 * @file Microstructure.cxx
 * @brief Implementation of the per-cell microstructure builder for the
 *        pre-LOCA steady state.  Joins the FRInput table (bulk fuel
 *        temperature) with the radial fission-gas-release table (local
 *        burnup and stored gas) keyed by (FA_number, Ax_slice).
 */

#include "tfgr/Microstructure.hxx"
#include "tfgr/Eos.hxx"

#include <string>
#include <unordered_map>
#include <vector>

namespace tfgr {

namespace {

/**
 * FRInput stores the bulk fuel temperature TBM in degrees Celsius; the
 * integrator works in kelvin.
 */
constexpr double celsius_to_kelvin_offset = 273.15;

/** Build the join key used for (FA_number, Ax_slice) lookups. */
std::string joinKey(const std::string& fa_number, int ax_slice)
{
    return fa_number + ":" + std::to_string(ax_slice);
}

}  // namespace

std::vector<MicrostructureCell> buildMicrostructures(
    const std::vector<FRRow>& fr_rows,
    const std::vector<RadFGRow>& fg_rows)
{
    // Index FRInput by (FA_number, Ax_slice) once so the join below is O(N)
    // instead of scanning fr_rows for every radial node.
    std::unordered_map<std::string, const FRRow*> fr_index;
    fr_index.reserve(fr_rows.size());
    for (const FRRow& fr : fr_rows)
        fr_index[joinKey(fr.fa_number, fr.ax_slice)] = &fr;

    std::vector<MicrostructureCell> cells;
    cells.reserve(fg_rows.size());

    for (const RadFGRow& fg : fg_rows) {
        const auto match = fr_index.find(joinKey(fg.fa_number, fg.ax_slice));
        // FRInput may omit some (FA, Ax_slice) combinations; without a bulk
        // temperature we cannot initialise the cell, so skip it silently.
        if (match == fr_index.end())
            continue;

        const FRRow& fr = *match->second;

        MicrostructureCell cell;
        cell.fa_number  = fg.fa_number;
        cell.core_pos   = fg.core_pos;
        cell.ax_slice   = fg.ax_slice;
        cell.rad_node   = fg.rad_node;
        cell.rel_rad    = fg.rel_rad;
        cell.burnup     = fg.rad_burn;
        cell.gas_molar_density =
            fg.rad_gas_s * umol_per_mm3_to_mol_per_m3();
        cell.t_initial_K    = fr.tbm + celsius_to_kelvin_offset;
        cell.pore_radius_m  = defaultPoreRadius_m();
        cell.porosity       = defaultPorosity();
        cell.hbs_fraction   = defaultHbsFraction(fg.rad_burn);

        cells.push_back(cell);
    }

    return cells;
}

double defaultHbsFraction(double burnup_mwd_per_tM)
{
    constexpr double bu_no_hbs   = 30.0;  // MWd/tM, below: no HBS expected
    constexpr double bu_max_hbs  = 80.0;  // MWd/tM, above: upper bound
    constexpr double hbs_max     = 0.08;  // Cappia 2016 upper bound, VVER-1000

    if (burnup_mwd_per_tM <= bu_no_hbs)
        return 0.0;
    if (burnup_mwd_per_tM >= bu_max_hbs)
        return hbs_max;

    const double ramp =
        (burnup_mwd_per_tM - bu_no_hbs) / (bu_max_hbs - bu_no_hbs);
    return ramp * hbs_max;
}

double defaultPoreRadius_m()
{
    // TODO(cappia2016): replace with the burnup-dependent Cappia 2016 fit.
    return 0.5e-6;  // 0.5 um
}

double defaultPorosity()
{
    // TODO(cappia2016): replace with the publication-grade Cappia 2016 fit.
    return 0.05;  // 5 %
}

}  // namespace tfgr
