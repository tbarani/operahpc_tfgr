

#include "tfgr/InputReader.hxx"
#include <fstream>
#include <iostream>
#include <sstream>
#include <algorithm>

Config parseConfig(const std::string& filename){
    std::ifstream file(filename);

    if (!file.is_open()){
        std::cout << "Error opening file " << filename << std::endl;
        exit(1);
    }

    auto cfg = Config{};
    bool has_model = false;
    bool has_porosity = false;
    bool has_radius = false;

    std::string line;
    while (std::getline(file, line)) {
        if (!line.empty() && line.back() == '\r') 
            line.pop_back();
        if (line.empty() || line[0] == '#') 
            continue;
        
        auto eq = line.find('=');
        if (eq == std::string::npos)
            continue;
        //tokenize the string with the =
        const std::string key   = line.substr(0, eq);
        const std::string value = line.substr(eq + 1);

        if (key == "model") {
            if (value != "Delauney" && value != "NN"){
                std::cout << 
                    "Unknown model: \"" + value
                    + "\". Expected 'Delauney' or 'NN'.\n";
                exit(1);
            }
            cfg.model_name = value;
            has_model = true;
        } else if (key == "porosity") {
            cfg.porosity = std::stod(value);
            if (cfg.porosity < 0.0 || cfg.porosity > 1.0){
                std::cout << "porosity must be in [0, 1].";
                exit(1);
            }
            has_porosity = true;
        } else if (key == "radius") {
            cfg.radius_um = std::stod(value);
            has_radius = true;
        } else if (key == "nn_model")
            cfg.nn_model_path = value;
    }

    if (!has_model){
        std::cout << "Missing 'model' key in input file.\n";
        exit(1);
    }
    if (!has_porosity){
        std::cout << "Missing 'porosity' key in input file.\n";
        exit(1);
    }
    if (!has_radius){
        std::cout << "Missing 'radius' key in input file.\n";
        exit(1);
    }
    if (cfg.model_name == "NN" && cfg.nn_model_path.empty()){
        std::cout << "Missing 'nn_model' key in input file (required for model=NN).\n";
        exit(1);
    }

    return cfg;
}

static void trimLeft(std::string& s)
{
    s.erase(s.begin(),
            std::find_if(s.begin(), s.end(),
                         [](unsigned char c){ return !std::isspace(c); }));
}

std::vector<DataPoint> parseCSV(const std::string& filename)
{
    std::ifstream file(filename);
    if (!file.is_open()){
        std::cout << "Error opening file " << filename << std::endl;
        exit(1);
    }

    std::vector<DataPoint> data;
    std::string line;
    bool first_data_row = true;

    while (std::getline(file, line)) {
        if (!line.empty() && line.back() == '\r') 
            line.pop_back();
        if (line.empty() || line[0] == '#')
            continue;

        std::istringstream ss(line);
        std::string tok1, tok2;
        if (!std::getline(ss, tok1, ',') || !std::getline(ss, tok2, ','))
            continue;

        trimLeft(tok1);
        trimLeft(tok2);

        if (first_data_row) {
            first_data_row = false;
            try { std::stod(tok1); }
            catch (...) {
                // skip first row 
                continue; 
            }
        }

        data.push_back({std::stod(tok1), std::stod(tok2)});
    }

    if (data.size() < 2){
        std::cout << "CSV must contain at least two data rows.\n";
        exit(1);
    }

    //sort the read datas in case they are not already 
    std::sort(data.begin(), data.end(),
              [](const DataPoint& a, const DataPoint& b){
                  return a.time_s < b.time_s;
              });

    return data;
}

