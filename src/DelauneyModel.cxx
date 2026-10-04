
#include "tfgr/DelauneyModel.hxx"

// TODO(mmm_db): same placeholder.
double DelauneyModel::computeIncrement(const double dt, const double T_begin, const double T_end) const {
    double dFGR = 0.;
    return dFGR;
};

// TODO(mmm_db): query the Delaunay triangulation once the MMM surrogate database (r, α, P_norm, FGR) is available; until then this returns 0.0.
double DelauneyModel::computeIncrementLOCA(double /*dt*/,
                                           double /*T_begin*/,
                                           double /*T_end*/,
                                           double /*pore_pressure_Pa*/,
                                           double /*porosity*/,
                                           double /*pore_radius_um*/) const {
    return 0.0;
}
