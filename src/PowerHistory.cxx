#include "tfgr/PowerHistory.hxx"

#include <cctype>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>

namespace tfgr {

namespace {

// Strip trailing '\r' (CRLF tolerance, matching the CSV loader convention).
void stripCR(std::string& s)
{
    if (!s.empty() && s.back() == '\r') {
        s.pop_back();
    }
}

bool parseDoubleStrict(const std::string& tok, double& out)
{
    try {
        std::size_t consumed = 0;
        out = std::stod(tok, &consumed);
        return consumed == tok.size();
    } catch (...) {
        return false;
    }
}

// Pull the whitespace-delimited value following `key` out of `s`.
std::string extractField(const std::string& s, const std::string& key)
{
    std::size_t p = s.find(key);
    if (p == std::string::npos) {
        return std::string();
    }
    p += key.size();
    while (p < s.size() && std::isspace(static_cast<unsigned char>(s[p]))) {
        ++p;
    }
    std::size_t e = p;
    while (e < s.size() && !std::isspace(static_cast<unsigned char>(s[e]))) {
        ++e;
    }
    return s.substr(p, e - p);
}

std::vector<std::string> tokenize(const std::string& line)
{
    std::istringstream iss(line);
    std::vector<std::string> tok;
    std::string piece;
    while (iss >> piece) {
        tok.push_back(piece);
    }
    return tok;
}

}  // namespace

PowerCurve parsePowerHistory(const std::string& filename)
{
    std::ifstream file(filename);
    if (!file.is_open()) {
        std::cout << "Error opening power-history file " << filename << std::endl;
        std::exit(1);
    }

    // Read every meaningful line first so the number of data steps is known
    // up front and the vectors can be reserved exactly.
    std::vector<std::string> records;
    std::string line;
    while (std::getline(file, line)) {
        stripCR(line);
        if (line.empty() || line[0] == '#') {
            continue;
        }
        records.push_back(line);
    }

    // line 1 (FA / core pos) + line 2 (columns) + line 3 (units) + >= 1 data row.
    if (records.size() < 4u) {
        std::cout << "Power-history file " << filename
                  << " is too short: expected a 3-line header and at least one "
                     "data row.\n";
        std::exit(1);
    }

    PowerCurve curve;
    curve.fa_number = extractField(records[0], "FA:");
    curve.core_pos  = extractField(records[0], "Core_pos:");

    // Detect the axial resolution from the column header: the number of
    // Burnup_NN tokens between 'Time' and the first 'LHGR_01' token.
    const std::vector<std::string> cols = tokenize(records[1]);
    std::size_t lhgr_idx = cols.size();
    for (std::size_t i = 0; i < cols.size(); ++i) {
        if (cols[i].compare(0u, 5u, "LHGR_") == 0) {
            lhgr_idx = i;
            break;
        }
    }
    if (lhgr_idx == cols.size() || lhgr_idx < 2u) {
        std::cout << "Power-history file " << filename
                  << ": malformed column header (cannot locate LHGR columns).\n";
        std::exit(1);
    }
    curve.n_nodes = lhgr_idx - 1u;

    const std::size_t expected = 1u + 3u * curve.n_nodes;
    if (cols.size() != expected) {
        std::cout << "Power-history file " << filename
                  << ": column header has " << cols.size()
                  << " tokens but " << expected << " were expected for "
                  << curve.n_nodes << " axial nodes.\n";
        std::exit(1);
    }

    // records[2] holds the units; data starts at records[3].
    const std::size_t n_steps = records.size() - 3u;
    curve.t_efph.reserve(n_steps);
    curve.burnup.reserve(n_steps * curve.n_nodes);
    curve.lhgr.reserve(n_steps * curve.n_nodes);
    curve.fnflux.reserve(n_steps * curve.n_nodes);

    for (std::size_t s = 0; s < n_steps; ++s) {
        const std::vector<std::string> tok = tokenize(records[3u + s]);
        if (tok.size() != expected) {
            std::cout << "Power-history file " << filename
                      << ": data row " << s
                      << " has " << tok.size() << " tokens but " << expected
                      << " were expected for " << curve.n_nodes
                      << " axial nodes.\n";
            std::exit(1);
        }

        double t = 0.0;
        if (!parseDoubleStrict(tok[0], t)) {
            std::cout << "Power-history file " << filename
                      << ": non-numeric time '" << tok[0] << "' in data row "
                      << s << ".\n";
            std::exit(1);
        }
        curve.t_efph.push_back(t);

        for (std::size_t n = 0; n < curve.n_nodes; ++n) {
            double b = 0.0;
            if (!parseDoubleStrict(tok[1u + n], b)) {
                std::cout << "Power-history file " << filename
                          << ": non-numeric burnup '" << tok[1u + n]
                          << "' in data row " << s << ".\n";
                std::exit(1);
            }
            curve.burnup.push_back(b);
        }

        for (std::size_t n = 0; n < curve.n_nodes; ++n) {
            double p = 0.0;
            if (!parseDoubleStrict(tok[1u + curve.n_nodes + n], p)) {
                std::cout << "Power-history file " << filename
                          << ": non-numeric LHGR '" << tok[1u + curve.n_nodes + n]
                          << "' in data row " << s << ".\n";
                std::exit(1);
            }
            curve.lhgr.push_back(p);
        }

        for (std::size_t n = 0; n < curve.n_nodes; ++n) {
            double f = 0.0;
            if (!parseDoubleStrict(tok[1u + 2u * curve.n_nodes + n], f)) {
                std::cout << "Power-history file " << filename
                          << ": non-numeric FNFlux '"
                          << tok[1u + 2u * curve.n_nodes + n]
                          << "' in data row " << s << ".\n";
                std::exit(1);
            }
            curve.fnflux.push_back(f);
        }
    }

    return curve;
}

}  // namespace tfgr
