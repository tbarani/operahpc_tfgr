#pragma once

/**
 * @file Microstructure.hxx
 * @brief Per-cell fuel microstructure state and builder from initial loaders.
 *
 * A MicrostructureCell describes one (FA_number, Ax_slice, Rad_node) of the
 * pellet during the steady state right before the LOCA starts.  It carries
 * everything the time-integration driver needs as initial conditions:
 *
 *   - radial position                  Rel_rad               [-]
 *   - local burnup                     rad_burn              [MWd / tM]
 *   - molar gas density in the pore    (Rad_gas_s converted)  [mol/m^3]
 *   - steady-state bulk fuel T         (FRInput.TBM)          [K]
 *   - pore radius                      Cappia_2016 default   [m]
 *   - porosity                         Cappia_2016 default   [-]
 *   - HBS volume fraction              Barani_2020 default   [-]
 *
 * The last three quantities are not present in the MS-13 deliverables
 * and are filled with placeholders pending publication-grade defaults.
 * A `// TODO` marks each one.
 */

#include "tfgr/FRInput.hxx"
#include "tfgr/RadFG.hxx"

#include <string>
#include <vector>

namespace tfgr {

struct MicrostructureCell {
    std::string fa_number;       // "_00415"
    std::string core_pos;        // "2,1"
    int         ax_slice{};      // 1-based
    int         rad_node{};      // 1-based, 1..23
    double      rel_rad{};       // [-], 0..1
    double      burnup{};        // MWd / tM
    double      gas_molar_density{};  // mol / m^3 (from Rad_gas_s [umol/mm^3])
    double      t_initial_K{};        // K (steady-state pre-LOCA bulk fuel T)
    double      pore_radius_m{};      // m; default from Cappia 2016 (TODO)
    double      porosity{};           // [-]; default from Cappia 2016 (TODO)
    double      hbs_fraction{};       // [-]; default from Barani 2020 (TODO)
};

/**
 * Build the per-cell initial state for a (FA, axial-slice, radial-node)
 * selection by joining FRInput and RadFG.  Cells where the FA/axial-slice
 * combination is missing from FRInput are skipped (the FRInput file is
 * indexed by axial slice; if a slice is missing we cannot assign a
 * steady-state bulk temperature to it).
 *
 * @param fr_rows  Output of parseFRInput on the EOC/MOC/BOC table.
 * @param fg_rows  Output of parseRadFG on the matching rad_FG table.
 * @return         One MicrostructureCell per (FA, Ax_slice, Rad_node)
 *                 row in @p fg_rows that has a matching FRInput row.
 */
std::vector<MicrostructureCell> buildMicrostructures(
    const std::vector<FRRow>& fr_rows,
    const std::vector<RadFGRow>& fg_rows);

/**
 * Library-default value of the local HBS volume fraction as a function of
 * burnup.  Linear placeholder pending Barani 2020 / D5.2 publication-grade
 * fit.  Returns 0 below 30 MWd/tM (no HBS expected), ramps to 0.08 at
 * 80 MWd/tM (≈ Cappia 2016 upper bound for VVER-1000).
 *
 * TODO(barani2020): replace with the publication-grade parameterisation
 *                  from Barani et al. (2020) once it lands.
 */
double defaultHbsFraction(double burnup_mwd_per_tM);

/**
 * Library-default pore radius (metres) for HBS bubbles.  Placeholder
 * pending Cappia 2016 / D5.2 publication-grade parameterisation.
 *
 * TODO(cappia2016): replace with the publication-grade Cappia 2016 fit
 *                  (radius as a function of burnup).
 */
double defaultPoreRadius_m();

/**
 * Library-default porosity (volume fraction of bubbles in the HBS
 * matrix).  Placeholder pending Cappia 2016 / D5.2.
 *
 * TODO(cappia2016): replace with the publication-grade Cappia 2016 fit.
 */
double defaultPorosity();

}  // namespace tfgr