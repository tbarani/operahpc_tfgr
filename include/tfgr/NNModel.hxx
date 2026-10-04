#pragma once

/**
 * @file NNModel.hxx
 * @brief Neural-network surrogate for fission gas release (T73, slide 6).
 *
 * Topology (from MMM "22-CEA-Task-6.2.pptx", slide 6):
 *
 *      inputs  : [pore_radius_um, porosity, P_norm]     (3 neurons)
 *      hidden  : 100 neurons, ReLU
 *      output  : 1 neuron, sigmoid   ->  FGR in [0, 1]
 *
 * where P_norm = pore_pressure_Pa / 1.0e6 (RELAP `.5tm` reports Pa, the MMM
 * database used MPa).
 *
 * This is a from-scratch, stdlib-only inference implementation: no LibTorch,
 * ONNX, protobuf or any other third-party ML runtime is used.  Until the MMM
 * training set lands the weights are default-initialised to zero and the
 * forward pass is compiled but gated behind a placeholder return.
 */

#include "tfgr/BaseTFGRModel.hxx"

#include <string>
#include <vector>

namespace tfgr {

class NNModel : public BaseTFGRModel {
public:
    /**
     * @param weights_path  Optional path to a text weights file.  If the file
     *                      does not exist (or is empty) the model silently
     *                      keeps its default-initialised weights; the first
     *                      end-to-end run is expected to have no such file.
     */
    explicit NNModel(const std::string& weights_path);

    /// Legacy CSV-mode path.  No-op stub for now (no pore-pressure inputs).
    double computeIncrement(double dt,
                            double T_begin,
                            double T_end) const override;

    /// LOCA-mode path: runs the 3-100-1 MLP forward pass.
    /// `porosity` is unnamed in the declaration to avoid -Wshadow against
    /// BaseTFGRModel::porosity; the .cxx definition uses `local_porosity`.
    double computeIncrementLOCA(double dt,
                                double T_begin,
                                double T_end,
                                double pore_pressure_Pa,
                                double /*porosity*/,
                                double pore_radius_um) const override;

private:
    // Flat, row-major weight storage:
    //   w_hidden_ : kNumInputs * kNumHidden (300).  Hidden neuron h's weight
    //               for input i lives at w_hidden_[h * kNumInputs + i], with
    //               inputs ordered [pore_radius_um, porosity, P_norm].
    //   b_hidden_ : kNumHidden (100), one bias per hidden neuron.
    //   w_output_ : kNumHidden * kNumOutputs (100), w_output_[h].
    //   b_output_ : kNumOutputs (1).
    std::vector<double> w_hidden_;
    std::vector<double> b_hidden_;
    std::vector<double> w_output_;
    std::vector<double> b_output_;

    /// Load weights from `weights_path`; keep defaults on a missing/empty file.
    void loadWeights(const std::string& weights_path);
};

}  // namespace tfgr
