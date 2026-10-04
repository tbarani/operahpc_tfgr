#pragma once

/**
 * @file BaseTFGRModel.hxx
 * @brief Abstract base class for all tfgr models.
 */

class BaseTFGRModel {
public:
    double porosity;
    double radius_um;

    BaseTFGRModel(double porosity_, double radius_um_)
        : porosity(porosity_), radius_um(radius_um_) {}

    virtual ~BaseTFGRModel() = default;

    /**
     * Compute the increment of the FGR over a time step (legacy CSV-driven
     * path used by Main.cxx::runLegacyCsvFlow).
     *
     * @param dt       Time increment            [s]
     * @param T_begin  Temperature at step start  [K]
     * @param T_end    Temperature at step end    [K]
     * @return         FGR
     */
    virtual double computeIncrement(double dt,
                                    double T_begin,
                                    double T_end) const = 0;

    /**
     * Compute the increment of the FGR over a LOCA time step.
     *
     * Unlike computeIncrement(), this entry point also carries the local
     * microstructure and pore-gas state produced by the V-B-C equation of
     * state, so models that need it (Delauney, NN) can evaluate the full
     * physics. This is the entry point called by the LOCA driver.
     *
     * @param dt                Time increment                      [s]
     * @param T_begin           Temperature at step start           [K]
     * @param T_end             Temperature at step end             [K]
     * @param pore_pressure_Pa  Pore gas pressure from the V-B-C EOS [Pa]
     * @param porosity          Local bubble fraction (pore volume) [-]
     * @param pore_radius_um    Effective bubble radius             [um]
     * @return                  FGR increment over the step
     */
    virtual double computeIncrementLOCA(double dt,
                                        double T_begin,
                                        double T_end,
                                        double pore_pressure_Pa,
                                        double porosity,
                                        double pore_radius_um) const = 0;
};
