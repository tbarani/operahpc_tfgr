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
     * Compute the increment of the FGR over a time step.
     *
     * @param dt       Time increment            [s]
     * @param T_begin  Temperature at step start  [K]
     * @param T_end    Temperature at step end    [K]
     * @return         FGR 
     */
    virtual double computeIncrement(double dt,
                                    double T_begin,
                                    double T_end) const = 0;
};
