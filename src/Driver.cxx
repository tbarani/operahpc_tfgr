/**
 * @file Driver.cxx
 * @brief LOCA time-integration driver (see include/tfgr/Driver.hxx).
 *
 * Geometry: each axial slice is an independent 1D radial stack integrated on
 * its own RELAP5 temperature column.  Slice step increments are aggregated to
 * the rod level, mass-weighted by each slice's releasable gas content.
 */

#include "tfgr/Driver.hxx"
#include "tfgr/Eos.hxx"

#include <cstdlib>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <map>
#include <unordered_map>

namespace tfgr {
namespace {

/** Report a fatal driver error and terminate (repo convention: std::cout + exit(1)). */
[[noreturn]] void fail(const std::string& msg)
{
    std::cout << msg << "\n";
    std::exit(1);
}

}  // namespace

DriverResult runLocaDriver(const RelapTransient& relap,
                           const std::vector<MicrostructureCell>& cells,
                           const BaseTFGRModel& model,
                           const DriverOptions& opts,
                           const std::string& out_csv)
{
    const std::size_t n_steps = relap.time_s.size();
    if (n_steps < 2) {
        fail("[Driver] RELAP transient has fewer than 2 steps; nothing to integrate.");
    }

    const std::size_t n_ax = relap.n_axial;
    if (n_ax == 0) {
        fail("[Driver] RELAP transient reports n_axial = 0; cannot index the axial arrays.");
    }
    if (relap.tfuel_surface_K.size() < n_steps * n_ax ||
        relap.pgap_Pa.size() < n_steps * n_ax) {
        fail("[Driver] RELAP arrays are shorter than time_s.size() * n_axial; file is inconsistent.");
    }

    // Group the cells of the requested FA by axial slice.  A std::map keeps the
    // slice iteration order deterministic (ascending ax_slice).
    std::map<int, std::vector<MicrostructureCell>> slice_cells;
    for (const MicrostructureCell& cell : cells) {
        if (cell.fa_number == opts.fa_number) {
            slice_cells[cell.ax_slice].push_back(cell);
        }
    }

    if (slice_cells.empty()) {
        fail("[Driver] no cells for FA '" + opts.fa_number + "'.");
    }

    std::vector<int> slices;
    slices.reserve(slice_cells.size());
    for (const auto& entry : slice_cells) {
        slices.push_back(entry.first);
    }

    // Validate that every slice maps to a valid RELAP5 axial node.
    for (const int s : slices) {
        if (s < 1 || static_cast<std::size_t>(s) > n_ax) {
            fail("[Driver] axial slice " + std::to_string(s) +
                 " is outside the RELAP5 axial range 1.." + std::to_string(n_ax) + ".");
        }
    }

    // Per-cell state, persistent across the whole time loop.
    //   prev_T_per_slice[s][i]  : cell i's temperature at the previous step  [K]
    //   cell_FGR_per_slice[s][i]: running total FGR released by cell i        [-]
    std::unordered_map<int, std::vector<double>> prev_T_per_slice;
    std::unordered_map<int, std::vector<double>> cell_FGR_per_slice;

    // Per-slice releasable gas weight: sum_i gas_molar_density_i * hbs_i.
    std::vector<double> slice_gas_weight(slices.size(), 0.0);
    double total_gas_weight = 0.0;

    for (std::size_t si = 0; si < slices.size(); ++si) {
        const int s = slices[si];
        const std::vector<MicrostructureCell>& v = slice_cells.at(s);

        prev_T_per_slice[s].resize(v.size());
        cell_FGR_per_slice[s].assign(v.size(), 0.0);

        double weight = 0.0;
        for (std::size_t i = 0; i < v.size(); ++i) {
            prev_T_per_slice[s][i] = v[i].t_initial_K;
            weight += v[i].gas_molar_density * v[i].hbs_fraction;
        }
        slice_gas_weight[si] = weight;
        total_gas_weight += weight;
    }

    // Mass-weighted mean of a per-node RELAP5 field across the FA's slices.
    auto rodWeightedMean = [&slices, &slice_gas_weight, n_ax, total_gas_weight](
                               const std::vector<double>& field,
                               std::size_t k,
                               double scale) -> double {
        if (total_gas_weight <= 0.0) {
            return 0.0;
        }
        double sum = 0.0;
        for (std::size_t si = 0; si < slices.size(); ++si) {
            const auto node = static_cast<std::size_t>(slices[si] - 1);
            sum += field[k * n_ax + node] * slice_gas_weight[si];
        }
        return scale * sum / total_gas_weight;
    };

    DriverResult res;
    res.time_s.reserve(n_steps);
    res.T_clad_K.reserve(n_steps);
    res.P_gap_MPa.reserve(n_steps);
    res.tFGR_step.reserve(n_steps);
    res.FGR_cumulative.reserve(n_steps);

    // Step 0: initial condition, no increment yet (the per-cell T_begin for the
    // first integrated step is the steady-state t_initial_K kept above).
    {
        res.time_s.push_back(relap.time_s[0]);
        res.T_clad_K.push_back(rodWeightedMean(relap.tfuel_surface_K, 0, 1.0));
        res.P_gap_MPa.push_back(rodWeightedMean(relap.pgap_Pa, 0, 1.0e-6));
        res.tFGR_step.push_back(0.0);
        res.FGR_cumulative.push_back(0.0);
    }

    double rod_cumulative_FGR = 0.0;

    for (std::size_t k = 1; k < n_steps; ++k) {
        const double dt = relap.time_s[k] - relap.time_s[k - 1];
        if (dt <= 0.0) {
            // RELAP5 output can re-order rows on re-init; skip non-monotonic steps.
            continue;
        }

        double rod_step_num = 0.0;   // sum_s slice_step_FGR_s * slice_gas_weight_s

        for (std::size_t si = 0; si < slices.size(); ++si) {
            const int s = slices[si];
            const std::vector<MicrostructureCell>& v = slice_cells.at(s);
            std::vector<double>& prev_T = prev_T_per_slice.at(s);
            std::vector<double>& cell_FGR = cell_FGR_per_slice.at(s);

            // This slice's own RELAP5 temperature column, at axial node s.
            const double T_slice =
                relap.tfuel_surface_K[k * n_ax + static_cast<std::size_t>(s - 1)];

            std::vector<double> cell_step_FGR(v.size(), 0.0);
            double hbs_weight_sum = 0.0;

            for (std::size_t i = 0; i < v.size(); ++i) {
                const MicrostructureCell& cell = v[i];

                // Uniform loading within the slice: the pore gas sees the
                // slice's cladding temperature.
                const double pore_T_K = T_slice;

                // Pore gas pressure from the Van Brutzel-Castelier EOS.
                const double pore_P_Pa = vanBrutzelCastelierPressure(
                    PoreGasState{pore_T_K, cell.gas_molar_density, cell.pore_radius_m});

                // Phase D: pass pore_pressure_Pa, porosity, and pore_radius_um
                // into the LOCA-mode method.  Both Delauney and NN overrides
                // currently gate on returning 0.0 (no trained MMM weights yet)
                // — the call path is exercised end-to-end so the wiring is
                // verified. The base-class computeIncrement (3-arg, legacy CSV
                // path) is intentionally not called here.
                const double dFGR = model.computeIncrementLOCA(
                    dt, prev_T[i], pore_T_K, pore_P_Pa,
                    cell.porosity, cell.pore_radius_m * 1.0e6);

                cell_step_FGR[i] = dFGR;
                cell_FGR[i] += dFGR;
                prev_T[i] = pore_T_K;

                hbs_weight_sum += cell.hbs_fraction;
            }

            // HBS-weighted radial mean of this slice's step increment.
            double slice_step_num = 0.0;
            for (std::size_t i = 0; i < v.size(); ++i) {
                slice_step_num += cell_step_FGR[i] * v[i].hbs_fraction;
            }
            const double slice_step_FGR =
                (hbs_weight_sum > 0.0) ? (slice_step_num / hbs_weight_sum) : 0.0;

            rod_step_num += slice_step_FGR * slice_gas_weight[si];
        }

        // Mass-weighted rod-level aggregation across slices.
        const double rod_step_FGR =
            (total_gas_weight > 0.0) ? (rod_step_num / total_gas_weight) : 0.0;
        rod_cumulative_FGR += rod_step_FGR;

        const double T_clad_K = rodWeightedMean(relap.tfuel_surface_K, k, 1.0);
        const double P_gap_MPa = rodWeightedMean(relap.pgap_Pa, k, 1.0e-6);

        res.time_s.push_back(relap.time_s[k]);
        res.T_clad_K.push_back(T_clad_K);
        res.P_gap_MPa.push_back(P_gap_MPa);
        res.tFGR_step.push_back(rod_step_FGR);
        res.FGR_cumulative.push_back(rod_cumulative_FGR);
    }

    res.FGR_final = rod_cumulative_FGR;

    std::ofstream out(out_csv);
    if (!out.is_open()) {
        fail("[Driver] cannot open output CSV: " + out_csv);
    }

    out << "time_s,T_clad_K,P_gap_MPa,tFGR_step,FGR_cumulative\n";
    for (std::size_t r = 0; r < res.time_s.size(); ++r) {
        out << std::fixed << std::setprecision(6)
            << res.time_s[r]         << ","
            << res.T_clad_K[r]       << ","
            << res.P_gap_MPa[r]      << ","
            << res.tFGR_step[r]      << ","
            << res.FGR_cumulative[r] << "\n";
    }

    if (!out) {
        fail("[Driver] failed while writing output CSV: " + out_csv);
    }

    return res;
}

}  // namespace tfgr
