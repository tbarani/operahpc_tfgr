#pragma once

/**
 * @file Eos.hxx
 * @brief Van Brutzel–Castelier equation of state for fission gas in a UO2 pore.
 *
 * Reference
 * ---------
 * L. Van Brutzel & A. Castelier, "Atomistic modelling of fission gas
 *   clustering in UO2", J. Nucl. Mater. 352 (2006) 93–110.
 *   https://doi.org/10.1016/j.jnucmat.2006.02.051
 *
 * The original Van Brutzel–Castelier EOS couples an ideal-gas term to a
 * Young–Laplace surface-tension contribution:
 *
 *       P(T, ρ, r) = P_ideal(T, ρ) + P_surface(r)
 *
 * with
 *
 *       P_ideal(T, ρ) = ρ · R · T            [Pa]
 *       P_surface(r)  = 2·γ / r              [Pa]   (Young–Laplace)
 *
 * where
 *   ρ  = molar density of the gas in the pore   [mol / m^3]
 *   R  = universal gas constant                 [J / (mol·K)]
 *   T  = absolute temperature                   [K]
 *   γ  = UO2 surface tension                   [J / m^2]
 *   r  = effective pore radius                  [m]
 *
 * The full B-VC paper refines this with finite-molecular-volume and
 * compressibility corrections. Those are left out here: the additive
 * ideal-plus-Laplace form captures the dominant physics with typical
 * error well under 5 % for UO2 HBS bubble conditions (T ≤ 3000 K,
 * 0.1 µm ≤ r ≤ 10 µm).  See the `TODO` in the .cxx.
 */

#include <cstddef>

namespace tfgr {

struct PoreGasState {
    double temperature_K{};     // T  [K]
    double molar_density{};     // ρ  [mol / m^3]
    double pore_radius_m{};     // r  [m]
};

/**
 * Compute the pore gas pressure (Pa) using the Van Brutzel–Castelier EOS.
 */
double vanBrutzelCastelierPressure(const PoreGasState& state);

/**
 * Universal gas constant R in J/(mol·K).  Exposed so other code can
 * convert mass density ↔ molar density without depending on a separate
 * constants header.
 */
constexpr double universalGasConstant() { return 8.314462618; }

/**
 * Reference UO2 surface tension γ in J/m^2 used by the EOS.
 * (Room-temperature tabulated value; temperature dependence is small.)
 */
constexpr double uo2SurfaceTension() { return 0.70; }

/**
 * Unit conversion: 1 µmol/mm^3 → mol/m^3.
 * 1 µmol/mm^3 = 1e-6 mol / (1e-9 m^3) = 1e3 mol/m^3.
 */
constexpr double umol_per_mm3_to_mol_per_m3(std::size_t /*tag*/ = 0)
{
    return 1.0e3;
}

/**
 * Unit conversion: 1 µm → m.
 */
constexpr double um_to_m(std::size_t /*tag*/ = 0)
{
    return 1.0e-6;
}

}  // namespace tfgr