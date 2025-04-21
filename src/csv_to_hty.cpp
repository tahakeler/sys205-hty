#include <fstream>
#include <sstream>
#include <vector>
#include <string>
#include <iostream>
#include <iomanip>
#include "../third_party/nlohmann/json.hpp"

using json = nlohmann::json;

void convert_from_csv_to_hty(std::string csv_file_path, std::string hty_file_path);

int main() {
    std::string csv_file_path, hty_file_path;

    std::cout << "Please enter the .csv file path:" << std::endl;
    std::cin >> csv_file_path;
    std::cout << "Please enter the .hty file path:" << std::endl;
    std::cin >> hty_file_path;

    convert_from_csv_to_hty(csv_file_path, hty_file_path);

    return 0;
}

void convert_from_csv_to_hty(std::string csv_file_path, std::string hty_file_path) {
    std::ifstream csv_file(csv_file_path);
    if (!csv_file.is_open()) {
        std::cerr << "Error opening CSV file.\n";
        return;
    }

    std::string line;
    std::vector<std::string> headers;
    std::vector<std::vector<std::string>> raw_data;

    // Read header
    if (std::getline(csv_file, line)) {
        std::stringstream ss(line);
        std::string cell;
        while (std::getline(ss, cell, ',')) {
            headers.push_back(cell);
        }
    }

    // Read rows
    while (std::getline(csv_file, line)) {
        std::stringstream ss(line);
        std::string cell;
        std::vector<std::string> row;
        while (std::getline(ss, cell, ',')) {
            row.push_back(cell);
        }
        raw_data.push_back(row);
    }
    csv_file.close();

    int num_columns = headers.size();
    int num_rows = raw_data.size();
    int offset = 0;

    std::ofstream hty_file(hty_file_path, std::ios::binary);
    if (!hty_file.is_open()) {
        std::cerr << "Error creating HTY file.\n";
        return;
    }

    // Prepare metadata
    json metadata;
    metadata["num_rows"] = num_rows;
    metadata["num_groups"] = 1;
    metadata["groups"] = json::array();
    json group;
    group["num_columns"] = num_columns;
    group["offset"] = 0;
    group["columns"] = json::array();

    std::vector<std::vector<int>> int_data(num_columns);

    // Store raw values as 4-byte integers/floats
    for (int col = 0; col < num_columns; col++) {
        bool is_float = false;
        for (int row = 0; row < num_rows; row++) {
            if (raw_data[row][col].find('.') != std::string::npos) {
                is_float = true;
                break;
            }
        }

        for (int row = 0; row < num_rows; row++) {
            std::string value = raw_data[row][col];
            if (is_float) {
                float f = std::stof(value);
                hty_file.write(reinterpret_cast<const char*>(&f), sizeof(float));
            } else {
                int i = std::stoi(value);
                hty_file.write(reinterpret_cast<const char*>(&i), sizeof(int));
            }
        }

        group["columns"].push_back({
            {"column_name", headers[col]},
            {"column_type", is_float ? "float" : "int"}
        });
    }

    metadata["groups"].push_back(group);

    std::string metadata_str = metadata.dump();
    hty_file.write(metadata_str.c_str(), metadata_str.size());

    int metadata_size = metadata_str.size();
    hty_file.write(reinterpret_cast<const char*>(&metadata_size), sizeof(int));

    hty_file.close();
    std::cout << "HTY file written successfully.\n";
}
