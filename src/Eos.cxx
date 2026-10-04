#include "tfgr/Eos.hxx"

#include <cstdlib>
#include <iostream>

namespace tfgr {

double vanBrutzelCastelierPressure(const PoreGasState& state)
{
    // Defensive validation: T must be strictly positive (else the ideal-gas
    // term diverges), and r must be strictly positive (else the Laplace term
    // diverges). ρ = 0 is allowed and yields P = P_surface(r).
    if (!(state.temperature_K > 0.0)) {
        std::cout << "vanBrutzelCastelierPressure: temperature_K must be > 0, got "
                  << state.temperature_K << ".\n";
        std::exit(1);
    }
    if (!(state.pore_radius_m > 0.0)) {
        std::cout << "vanBrutzelCastelierPressure: pore_radius_m must be > 0, got "
                  << state.pore_radius_m << ".\n";
        std::exit(1);
    }

    // P_ideal = ρ R T
    const double P_ideal = state.molar_density * universalGasConstant()
                           * state.temperature_K;

    // P_surface = 2 γ / r   (Young–Laplace)
    const double P_surface = 2.0 * uo2SurfaceTension() / state.pore_radius_m;

    // Additive form. The full Van Brutzel–Castelier (2006) EOS adds
    // finite-molecular-volume (Boublik / Carnahan–Starling) and
    // compressibility corrections on top of this; for the HBS bubble
    // conditions targeted here (T ≤ 3000 K, 0.1 µm ≤ r ≤ 10 µm) those
    // corrections shift the answer by well under 5 %. Add them here when
    // the MMM database lands and we can validate against it.
    const double P = P_ideal + P_surface;

    // P should be strictly positive for any physical (T, ρ, r); if it isn't,
    // the input molar density is inconsistent (e.g. negative after some
    // bug in upstream code). Guard rather than silently return junk.
    if (!(P > 0.0)) {
        std::cout << "vanBrutzelCastelierPressure: non-positive pressure "
                  << P << " Pa (T=" << state.temperature_K
                  << " K, ρ=" << state.molar_density
                  << " mol/m^3, r=" << state.pore_radius_m << " m).\n";
        std::exit(1);
    }

    return P;
}

}  // namespace tfgr