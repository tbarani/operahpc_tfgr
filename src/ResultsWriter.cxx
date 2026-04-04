
#include "tfgr/ResultsWriter.hxx"
#include <iomanip>

void ResultsWriter::writeRow(const double time_s, const double temperature, const double fgr){
    file_ << std::fixed << std::setprecision(6)
          << time_s      << ","
          << temperature << ","
          << fgr    << "\n";
}
;