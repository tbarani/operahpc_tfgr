#include "tfgr/Validator.hxx"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

namespace {

/// Trim leading and trailing whitespace in place.
void trim(std::string& s)
{
    const auto not_space = [](unsigned char c) {
        return !std::isspace(c);
    };
    s.erase(s.begin(), std::find_if(s.begin(), s.end(), not_space));
    s.erase(std::find_if(s.rbegin(), s.rend(), not_space).base(), s.end());
}

/// Parse @p tok as a full double (no trailing garbage).  Returns false on
/// failure, which is how header / non-numeric rows are detected.
bool tryParseDouble(const std::string& tok, double& out)
{
    if (tok.empty())
        return false;
    try {
        std::size_t pos = 0;
        out = std::stod(tok, &pos);
        return pos == tok.size();
    } catch (...) {
        return false;
    }
}

}  // namespace

namespace tfgr {

std::vector<TimeFGRPair> readFgrCsv(const std::string& path)
{
    std::ifstream file(path);
    if (!file.is_open()) {
        std::cout << "Cannot open CSV file: " + path << "\n";
        exit(1);
    }

    std::vector<TimeFGRPair> rows;
    std::string line;

    while (std::getline(file, line)) {
        if (!line.empty() && line.back() == '\r')
            line.pop_back();
        if (line.empty() || line[0] == '#')
            continue;

        std::istringstream ss(line);
        std::vector<std::string> tokens;
        std::string tok;
        while (std::getline(ss, tok, ','))
            tokens.push_back(tok);

        if (tokens.size() < 5)
            continue;

        trim(tokens[0]);
        trim(tokens[4]);

        double t   = 0.0;
        double fgr = 0.0;
        if (!tryParseDouble(tokens[0], t) || !tryParseDouble(tokens[4], fgr))
            continue;  // header or malformed row

        rows.push_back(TimeFGRPair{t, fgr});
    }

    return rows;
}

ValidationReport validateAgainstJerkvist(const std::string& our_csv,
                                         const std::string& ref_csv,
                                         double threshold)
{
    const std::vector<TimeFGRPair> ours = readFgrCsv(our_csv);
    const std::vector<TimeFGRPair> ref  = readFgrCsv(ref_csv);

    if (ours.size() < 2 || ref.size() < 2) {
        std::cout << "Validation requires at least two rows in each CSV.\n";
        exit(1);
    }

    ValidationReport report;
    report.n_ours = ours.size();
    report.n_ref  = ref.size();

    double max_abs_error = 0.0;
    double sum_abs_error = 0.0;

    for (const TimeFGRPair& row : ours) {
        const double t = row.time_s;

        // First reference sample whose time is strictly greater than t.
        const auto upper = std::upper_bound(
            ref.begin(), ref.end(), t,
            [](double value, const TimeFGRPair& p) {
                return value < p.time_s;
            });
        const std::size_t idx =
            static_cast<std::size_t>(upper - ref.begin());

        double fgr_ref = 0.0;
        if (idx == 0) {
            // t is before the first reference sample: clamp.
            fgr_ref = ref.front().fgr_cumulative;
        } else if (idx >= ref.size()) {
            // t is at or past the last reference sample: clamp.
            fgr_ref = ref.back().fgr_cumulative;
        } else {
            const TimeFGRPair& lo = ref[idx - 1];
            const TimeFGRPair& hi = ref[idx];
            const double dt = hi.time_s - lo.time_s;
            if (dt != 0.0) {
                const double w = (t - lo.time_s) / dt;
                fgr_ref = lo.fgr_cumulative
                        + (hi.fgr_cumulative - lo.fgr_cumulative) * w;
            } else {
                fgr_ref = lo.fgr_cumulative;
            }
        }

        const double err = std::fabs(row.fgr_cumulative - fgr_ref);
        if (err > max_abs_error)
            max_abs_error = err;
        sum_abs_error += err;
    }

    const double total = static_cast<double>(ours.size());

    report.max_abs_error  = max_abs_error;
    report.mean_abs_error = sum_abs_error / total;
    report.final_ours     = ours.back().fgr_cumulative;
    report.final_ref      = ref.back().fgr_cumulative;
    report.passed         = (max_abs_error <= threshold);

    return report;
}

}  // namespace tfgr
