#include "tfgr/NNModel.hxx"

#include <cmath>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>

namespace {

// Network dimensions -- must stay in sync with the weights-file header.
constexpr std::size_t kNumInputs  = 3;
constexpr std::size_t kNumHidden  = 100;
constexpr std::size_t kNumOutputs = 1;

// ReLU activation.
double relu(double x) {
    return x > 0.0 ? x : 0.0;
}

// Logistic sigmoid activation; output in (0, 1).
double sigmoid(double x) {
    return 1.0 / (1.0 + std::exp(-x));
}

// Row-major dense layer: out = W * x + b, with W stored as [rows x cols].
// The caller applies the activation afterwards.
void dense(const std::vector<double>& weights,
           const std::vector<double>& bias,
           const std::vector<double>& x,
           std::size_t rows,
           std::size_t cols,
           std::vector<double>& out) {
    out.resize(rows);
    for (std::size_t r = 0; r < rows; ++r) {
        double acc = bias[r];
        for (std::size_t c = 0; c < cols; ++c) {
            acc += weights[r * cols + c] * x[c];
        }
        out[r] = acc;
    }
}

}  // namespace

namespace tfgr {

NNModel::NNModel(const std::string& weights_path)
    : BaseTFGRModel(0.0, 0.0),  // base-class porosity_/radius_um unused by NN
      w_hidden_(kNumInputs * kNumHidden, 0.0),
      b_hidden_(kNumHidden, 0.0),
      w_output_(kNumHidden * kNumOutputs, 0.0),
      b_output_(kNumOutputs, 0.0) {
    loadWeights(weights_path);
}

void NNModel::loadWeights(const std::string& weights_path) {
    std::ifstream file(weights_path);
    if (!file.is_open()) {
        // No weights file yet: keep the deterministic zero initialisation.
        // This is the expected first-run case, so it is intentionally quiet.
        return;
    }

    std::vector<double> values;
    std::string line;
    while (std::getline(file, line)) {
        if (!line.empty() && line.back() == '\r') {
            line.pop_back();
        }
        if (line.empty() || line[0] == '#') {
            continue;  // header / comment line
        }
        values.push_back(std::stod(line));
    }

    if (values.empty()) {
        // Empty or comment-only file: keep default-init weights.
        return;
    }

    const std::size_t expected = kNumInputs * kNumHidden + kNumHidden
                                 + kNumHidden * kNumOutputs + kNumOutputs;
    if (values.size() != expected) {
        std::cout << "NNModel weights file \"" << weights_path << "\" has "
                  << values.size() << " numeric values, expected " << expected
                  << ".\n";
        std::exit(1);
    }

    std::size_t k = 0;
    for (std::size_t i = 0; i < w_hidden_.size(); ++i) w_hidden_[i] = values[k++];
    for (std::size_t i = 0; i < b_hidden_.size(); ++i) b_hidden_[i] = values[k++];
    for (std::size_t i = 0; i < w_output_.size(); ++i) w_output_[i] = values[k++];
    for (std::size_t i = 0; i < b_output_.size(); ++i) b_output_[i] = values[k++];
}

// TODO(mmm_db): implement the save path once training data exists.  Format
// should mirror loadWeights(): "# NNModel 3-100-1" then 501 values in the
// order w_hidden_, b_hidden_, w_output_, b_output_.

double NNModel::computeIncrement(double dt,
                                 double T_begin,
                                 double T_end) const {
    (void)dt;
    (void)T_begin;
    (void)T_end;
    // TODO(mmm_db): the legacy CSV path carries no pore-pressure/porosity
    // inputs for the surrogate; left as a no-op.
    return 0.0;
}

double NNModel::computeIncrementLOCA(double dt,
                                     double T_begin,
                                     double T_end,
                                     double pore_pressure_Pa,
                                     double local_porosity,
                                     double pore_radius_um) const {
    (void)dt;
    (void)T_begin;
    (void)T_end;

    // Input vector: [pore_radius_um, local_porosity, normalised pressure].
    // RELAP `.5tm` outputs Pa; the MMM database used MPa, hence / 1e6.
    const std::vector<double> x = {
        pore_radius_um,
        local_porosity,
        pore_pressure_Pa / 1.0e6,
    };

    // Forward pass -- compiled and executed so the helpers stay honest, but
    // the result is not yet used until trained weights replace the zeros.
    std::vector<double> hidden;
    dense(w_hidden_, b_hidden_, x, kNumHidden, kNumInputs, hidden);
    for (double& h : hidden) {
        h = relu(h);
    }

    std::vector<double> output;
    dense(w_output_, b_output_, hidden, kNumOutputs, kNumHidden, output);
    output[0] = sigmoid(output[0]);

    // TODO(mmm_db): uncomment the forward pass once trained weights are
    // available.
    (void)output;
    return 0.0;
}

}  // namespace tfgr
