#include "tfgr/FRInput.hxx"

#include <algorithm>
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

// Left-trim helper retained for future use; currently the per-line tokenizer
// below uses istringstream which already skips leading whitespace.
[[maybe_unused]] static void trimLeft(std::string& s)
{
    s.erase(s.begin(),
            std::find_if(s.begin(), s.end(),
                         [](unsigned char c) { return !std::isspace(c); }));
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

bool parseIntStrict(const std::string& tok, int& out)
{
    try {
        std::size_t consumed = 0;
        out = static_cast<int>(std::stol(tok, &consumed));
        return consumed == tok.size();
    } catch (...) {
        return false;
    }
}

}  // namespace

std::vector<FRRow> parseFRInput(const std::string& filename)
{
    std::ifstream file(filename);
    if (!file.is_open()) {
        std::cout << "Error opening FR input file " << filename << std::endl;
        std::exit(1);
    }

    std::vector<FRRow> rows;
    std::string line;

    // Skip the 2-line header (column names + units).
    int header_lines_to_skip = 2;
    while (header_lines_to_skip > 0 && std::getline(file, line)) {
        stripCR(line);
        --header_lines_to_skip;
    }
    if (header_lines_to_skip > 0) {
        std::cout << "FR input file " << filename
                  << " ended before the header was complete.\n";
        std::exit(1);
    }

    while (std::getline(file, line)) {
        stripCR(line);
        if (line.empty()) {
            continue;
        }

        // Tokenise on whitespace.
        std::istringstream iss(line);
        std::vector<std::string> tok;
        std::string piece;
        while (iss >> piece) {
            tok.push_back(piece);
        }
        if (tok.size() < 25u) {
            continue;  // tolerate blank/short separator lines
        }

        FRRow row;
        row.fa_number = tok[0];
        row.core_pos  = tok[1];
        if (!parseIntStrict(tok[2], row.ax_slice)) {
            std::cout << "FR input " << filename
                      << ": bad Ax_slice '" << tok[2] << "'.\n";
            std::exit(1);
        }

        // Numeric columns 3..25 (Ax_burnup .. GAPHTC).
        double* const slots[23] = {
            &row.ax_burnup, &row.rob, &row.rfd, &row.pgas, &row.xhe,
            &row.dba, &row.dhi, &row.d0, &row.dha, &row.oxt,
            &row.pow, &row.tbi, &row.tba, &row.tbm, &row.thi,
            &row.tha, &row.thm, &row.aftc, &row.ahtc, &row.sefl,
            &row.sefm, &row.sehl, &row.gaphtc
        };
        bool ok = true;
        for (std::size_t k = 0; k < 23u; ++k) {
            double v = 0.0;
            if (!parseDoubleStrict(tok[3u + k], v)) {
                std::cout << "FR input " << filename
                          << ": non-numeric field at column "
                          << (3u + k) << " ('" << tok[3u + k]
                          << "') in row '" << row.fa_number << "'.\n";
                ok = false;
                break;
            }
            *slots[k] = v;
        }
        if (!ok) {
            std::exit(1);
        }

        rows.push_back(row);
    }

    return rows;
}

}  // namespace tfgr