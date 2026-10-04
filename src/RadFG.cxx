#include "tfgr/RadFG.hxx"

#include <cstdlib>
#include <fstream>
#include <iostream>
#include <sstream>

namespace tfgr {

std::vector<RadFGRow> parseRadFG(const std::string& filename)
{
    std::ifstream file(filename);
    if (!file.is_open()) {
        std::cout << "Error opening file " << filename << std::endl;
        exit(1);
    }

    std::vector<RadFGRow> rows;
    std::string line;
    int header_lines = 2;  // 1: column names, 2: units

    while (std::getline(file, line)) {
        if (!line.empty() && line.back() == '\r')
            line.pop_back();
        if (line.empty())
            continue;
        if (header_lines > 0) {
            --header_lines;
            continue;
        }

        std::istringstream ss(line);
        std::vector<std::string> tok;
        std::string t;
        while (ss >> t)
            tok.push_back(t);

        if (tok.size() != 8u) {
            std::cout << "Malformed row in " << filename
                      << ": expected 8 tokens, got " << tok.size()
                      << ": " << line << std::endl;
            exit(1);
        }

        RadFGRow row;
        row.fa_number = tok[0];
        row.core_pos  = tok[1];
        row.ax_slice  = static_cast<int>(std::stol(tok[2]));
        row.rad_node  = static_cast<int>(std::stol(tok[3]));
        row.rel_rad   = std::stod(tok[4]);
        row.rad_burn  = std::stod(tok[5]);
        row.rad_gas_c = std::stod(tok[6]);
        row.rad_gas_s = std::stod(tok[7]);

        if (row.rel_rad < 0.0 || row.rel_rad > 1.0) {
            std::cout << "Warning: Rel_rad out of [0, 1] (" << row.rel_rad
                      << ") in " << filename << std::endl;
        }

        rows.push_back(row);
    }

    return rows;
}

}  // namespace tfgr
