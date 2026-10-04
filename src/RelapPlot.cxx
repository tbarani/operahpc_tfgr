#include "tfgr/RelapPlot.hxx"

#include <cctype>
#include <cstddef>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

namespace tfgr {

namespace {

constexpr std::size_t kExpectedColumns = 70u;
constexpr std::size_t kHeaderRows      = 4u;

// Strip a trailing '\r' (CRLF tolerance, matching the CSV loader convention).
void stripCR(std::string& s)
{
    if (!s.empty() && s.back() == '\r') {
        s.pop_back();
    }
}

// Index of the first non-whitespace character (== s.size() if all blank).
std::size_t firstNonSpace(const std::string& s)
{
    std::size_t i = 0;
    while (i < s.size()
           && std::isspace(static_cast<unsigned char>(s[i])) != 0) {
        ++i;
    }
    return i;
}

// Trim leading/trailing whitespace (header tokens are padded inside quotes).
std::string trim(const std::string& s)
{
    std::size_t b = 0;
    std::size_t e = s.size();
    while (b < e && std::isspace(static_cast<unsigned char>(s[b])) != 0) {
        ++b;
    }
    while (e > b && std::isspace(static_cast<unsigned char>(s[e - 1u])) != 0) {
        --e;
    }
    return s.substr(b, e - b);
}

// Extract every "..." substring from a header line. Header tokens are padded
// with spaces inside their quotes, so a whitespace split cannot be used and
// each extracted token is trimmed.
std::vector<std::string> extractQuoted(const std::string& line)
{
    std::vector<std::string> out;
    std::size_t i = 0;
    while (i < line.size()) {
        if (line[i] == '"') {
            const std::size_t start = ++i;
            while (i < line.size() && line[i] != '"') {
                ++i;
            }
            out.push_back(trim(line.substr(start, i - start)));
            if (i < line.size()) {
                ++i;  // skip the closing quote
            }
        } else {
            ++i;
        }
    }
    return out;
}

// Split an unquoted, whitespace-separated data row into tokens.
std::vector<std::string> splitWhitespace(const std::string& line)
{
    std::vector<std::string> out;
    std::istringstream iss(line);
    std::string piece;
    while (iss >> piece) {
        out.push_back(piece);
    }
    return out;
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

// "49100101" -> "0491": the leading assembly number precedes the fixed
// 5-character axial-node + channel-type suffix (e.g. "00101"); it is
// left-padded to the 4-digit form used by the display names ("Tfuel-0491").
std::string assemblyFromIdentifier(const std::string& id)
{
    if (id.size() <= 5u) {
        return id;
    }
    std::string prefix = id.substr(0, id.size() - 5u);
    while (prefix.size() < 4u) {
        prefix.insert(prefix.begin(), '0');
    }
    return prefix;
}

[[noreturn]] void fail(const std::string& filename, const std::string& msg)
{
    std::cout << "RELAP plot " << filename << ": " << msg << "\n";
    std::exit(1);
}

}  // namespace

RelapTransient parseRelapPlot(const std::string& filename)
{
    std::ifstream file(filename);
    if (!file.is_open()) {
        std::cout << "Error opening RELAP plot file " << filename << std::endl;
        std::exit(1);
    }

    // 70 columns = time_run + time_s + 3 groups of n_axial + 2 BALAST sentinels.
    RelapTransient out;
    out.n_axial = (kExpectedColumns - 4u) / 3u;  // 22

    std::size_t header_rows = 0;
    bool        have_id     = false;
    bool        have_data   = false;

    std::string line;
    while (std::getline(file, line)) {
        stripCR(line);
        const std::size_t p = firstNonSpace(line);
        if (p == line.size()) {
            continue;  // blank line
        }

        // Header lines begin (after optional leading spaces) with a quote.
        if (line[p] == '"') {
            if (header_rows < kHeaderRows) {
                if (header_rows == 1u) {  // identifier row
                    const std::vector<std::string> toks = extractQuoted(line);
                    if (toks.size() >= 3u) {
                        out.assembly_id = assemblyFromIdentifier(toks[2]);
                        have_id         = true;
                    }
                }
                ++header_rows;
            }
            continue;
        }

        // ---- Data row ----
        if (header_rows < kHeaderRows) {
            fail(filename, "data row before all 4 header rows were read.");
        }

        const std::vector<std::string> tok = splitWhitespace(line);
        if (tok.size() != kExpectedColumns) {
            fail(filename,
                 "expected " + std::to_string(kExpectedColumns)
                     + " columns but found " + std::to_string(tok.size())
                     + ".");
        }

        double v = 0.0;
        if (!parseDoubleStrict(tok[0], v)) {
            fail(filename, "non-numeric time_run ('" + tok[0] + "').");
        }
        out.time_run.push_back(v);

        if (!parseDoubleStrict(tok[1], v)) {
            fail(filename, "non-numeric time_s ('" + tok[1] + "').");
        }
        out.time_s.push_back(v);

        const std::size_t n = out.n_axial;
        for (std::size_t node = 0; node < n; ++node) {
            // surface: cols 2..23; center: cols 25..46; pgap: cols 48..69.
            if (!parseDoubleStrict(tok[2u + node], v)) {
                fail(filename, "non-numeric tfuel_surface at column "
                                   + std::to_string(2u + node) + " ('"
                                   + tok[2u + node] + "').");
            }
            out.tfuel_surface_K.push_back(v);

            if (!parseDoubleStrict(tok[25u + node], v)) {
                fail(filename, "non-numeric tfuel_center at column "
                                   + std::to_string(25u + node) + " ('"
                                   + tok[25u + node] + "').");
            }
            out.tfuel_center_K.push_back(v);

            if (!parseDoubleStrict(tok[48u + node], v)) {
                fail(filename, "non-numeric pgap at column "
                                   + std::to_string(48u + node) + " ('"
                                   + tok[48u + node] + "').");
            }
            out.pgap_Pa.push_back(v);
        }

        have_data = true;
    }

    if (header_rows < kHeaderRows) {
        fail(filename, "file ended before the 4 header rows were complete.");
    }
    if (!have_id) {
        fail(filename, "could not read the assembly identifier from header row 2.");
    }
    if (!have_data) {
        fail(filename, "no data rows found.");
    }

    return out;
}

}  // namespace tfgr
