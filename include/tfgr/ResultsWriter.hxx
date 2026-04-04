#pragma once
#include <fstream>
#include <string>
#include <iostream>

/**
 * @file ResultsWriter.hxx
 * @brief Writer for the csv output file.
 */

class ResultsWriter {
public:
    /**
     * Open @p filename and write the CSV header.
     */
    ResultsWriter(const std::string& filename) : file_(filename){
        if (!file_.is_open()){
            std::cout << "Cannot open output file: " + filename;
            exit(1);
        }

        file_ << "time (s),temperature (K),FGR (/)\n";
    };

    ResultsWriter(const ResultsWriter&)            = delete;
    ResultsWriter& operator=(const ResultsWriter&) = delete;

    /**
     * Append one row to the CSV.
     *
     * @param time_s       Time of the current step         [s]
     * @param temperature  Temperature at the current step  [K]
     * @param quantity     FGR this time step  [/]
     */
    void writeRow(const double time_s, const double temperature, const double fgr);

private:
    std::ofstream file_;
};