#pragma once
#include "tfgr/BaseTFGRModel.hxx"

/**
 * @file DelauneyModel.hxx
 * @brief TFGR model using the Delauney triangulation to interpolate.
 */

class DelauneyModel : public BaseTFGRModel{
public:
    DelauneyModel(const double porosity_, const double radius_) 
    : BaseTFGRModel(porosity_, radius_){};
    double computeIncrement(const double dt, const double T_begin, const double T_end) const override;

}
;
