#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <cstring>
#include <iomanip>
#include <cmath>
#include "../third_party/nlohmann/json.hpp"

using json = nlohmann::json;

// Task 2: Extract metadata from HTY file
nlohmann::json extract_metadata(std::string hty_file_path) {
    std::ifstream file(hty_file_path, std::ios::binary | std::ios::ate);
    int size = file.tellg();
    file.seekg(size - sizeof(int));
    int metadata_size;
    file.read(reinterpret_cast<char*>(&metadata_size), sizeof(int));
    file.seekg(size - sizeof(int) - metadata_size);
    std::string metadata_str(metadata_size, '\0');
    file.read(&metadata_str[0], metadata_size);
    return json::parse(metadata_str);
}

// Task 3.1: Project a single column
std::vector<int> project_single_column(json metadata, std::string hty_file_path, std::string projected_column) {
    std::ifstream file(hty_file_path, std::ios::binary);
    std::vector<int> result;

    int num_rows = metadata["num_rows"];
    for (auto& group : metadata["groups"]) {
        int offset = group["offset"];
        int col_index = -1;
        bool is_float = false;

        for (int i = 0; i < group["columns"].size(); ++i) {
            if (group["columns"][i]["column_name"] == projected_column) {
                col_index = i;
                is_float = (group["columns"][i]["column_type"] == "float");
                break;
            }
        }

        if (col_index == -1) continue;

        int row_offset = 0;
        for (int i = 0; i < col_index; ++i)
            row_offset += 4 * num_rows;

        file.seekg(offset + row_offset);
        for (int i = 0; i < num_rows; ++i) {
            if (is_float) {
                float val;
                file.read(reinterpret_cast<char*>(&val), sizeof(float));
                result.push_back(*reinterpret_cast<int*>(&val));
            } else {
                int val;
                file.read(reinterpret_cast<char*>(&val), sizeof(int));
                result.push_back(val);
            }
        }
    }

    return result;
}

// Task 3.2: Display a single column
void display_column(json metadata, std::string column_name, std::vector<int> data) {
    std::cout << column_name << std::endl;
    for (auto& group : metadata["groups"]) {
        for (auto& col : group["columns"]) {
            if (col["column_name"] == column_name) {
                std::string type = col["column_type"];
                for (int v : data) {
                    if (type == "float") {
                        float f;
                        std::memcpy(&f, &v, sizeof(float));
                        std::cout << f << std::endl;
                    } else {
                        std::cout << v << std::endl;
                    }
                }
            }
        }
    }
}

// Task 4: Filter on a single column
std::vector<int> filter(json metadata, std::string hty_file_path, std::string projected_column, int operation, int filtered_value) {
    std::vector<int> column_data = project_single_column(metadata, hty_file_path, projected_column);
    std::vector<int> result;

    for (int val : column_data) {
        bool pass = false;
        switch (operation) {
            case 0: pass = (val > filtered_value); break;
            case 1: pass = (val >= filtered_value); break;
            case 2: pass = (val < filtered_value); break;
            case 3: pass = (val <= filtered_value); break;
            case 4: pass = (val == filtered_value); break;
            case 5: pass = (val != filtered_value); break;
        }
        if (pass) result.push_back(val);
    }

    return result;
}

// Task 5.1: Project multiple columns
std::vector<std::vector<int>> project(json metadata, std::string hty_file_path, std::vector<std::string> projected_columns) {
    std::vector<std::vector<int>> result;
    for (const std::string& column : projected_columns) {
        result.push_back(project_single_column(metadata, hty_file_path, column));
    }
    return result;
}

// Task 5.2: Display result set (multiple columns)
void display_result_set(json metadata, std::vector<std::string> column_names, std::vector<std::vector<int>> result_set) {
    for (size_t i = 0; i < column_names.size(); ++i) {
        std::cout << column_names[i];
        if (i != column_names.size() - 1) std::cout << ",";
    }
    std::cout << std::endl;

    int num_rows = result_set[0].size();
    for (int i = 0; i < num_rows; ++i) {
        for (size_t j = 0; j < result_set.size(); ++j) {
            bool is_float = false;
            for (auto& group : metadata["groups"]) {
                for (auto& col : group["columns"]) {
                    if (col["column_name"] == column_names[j] && col["column_type"] == "float") {
                        is_float = true;
                    }
                }
            }
            if (is_float) {
                float f;
                std::memcpy(&f, &result_set[j][i], sizeof(float));
                std::cout << f;
            } else {
                std::cout << result_set[j][i];
            }
            if (j != result_set.size() - 1) std::cout << ",";
        }
        std::cout << std::endl;
    }
}

// Task 6: Project and filter on same group
std::vector<std::vector<int>> project_and_filter(json metadata, std::string hty_file_path, std::vector<std::string> projected_columns, std::string filtered_column, int op, int value) {
    std::vector<int> filter_col = project_single_column(metadata, hty_file_path, filtered_column);
    std::vector<std::vector<int>> projected_data = project(metadata, hty_file_path, projected_columns);
    std::vector<std::vector<int>> result(projected_columns.size());

    for (int i = 0; i < filter_col.size(); ++i) {
        bool pass = false;
        switch (op) {
            case 0: pass = (filter_col[i] > value); break;
            case 1: pass = (filter_col[i] >= value); break;
            case 2: pass = (filter_col[i] < value); break;
            case 3: pass = (filter_col[i] <= value); break;
            case 4: pass = (filter_col[i] == value); break;
            case 5: pass = (filter_col[i] != value); break;
        }
        if (pass) {
            for (int j = 0; j < projected_columns.size(); ++j) {
                result[j].push_back(projected_data[j][i]);
            }
        }
    }
    return result;
}

// Task 7: Add rows to .hty file
void add_row(json metadata, std::string hty_file_path, std::string modified_hty_file_path, std::vector<std::vector<int>> rows) {
    std::ifstream in(hty_file_path, std::ios::binary);
    in.seekg(0, std::ios::end);
    int size = in.tellg();
    in.seekg(0);

    int metadata_size;
    in.seekg(size - sizeof(int));
    in.read(reinterpret_cast<char*>(&metadata_size), sizeof(int));

    int raw_data_size = size - metadata_size - sizeof(int);
    std::vector<char> raw_data(raw_data_size);
    in.seekg(0);
    in.read(raw_data.data(), raw_data_size);
    in.close();

    int num_new_rows = rows[0].size();
    metadata["num_rows"] = metadata["num_rows"].get<int>() + num_new_rows;

    std::ofstream out(modified_hty_file_path, std::ios::binary);
    out.write(reinterpret_cast<const char*>(raw_data.data()), raw_data_size);

    for (int col = 0; col < rows.size(); ++col) {
        for (int row = 0; row < rows[col].size(); ++row) {
            out.write(reinterpret_cast<const char*>(&rows[col][row]), sizeof(int));
        }
    }

    std::string new_metadata_str = metadata.dump();
    out.write(new_metadata_str.c_str(), new_metadata_str.size());
    int new_size = new_metadata_str.size();
    out.write(reinterpret_cast<const char*>(&new_size), sizeof(int));
    out.close();
}

int main() {
    std::string hty_file_path = "test.hty";
    std::string modified_hty_path = "test_modified.hty";

    // Step 1: Extract metadata
    json meta = extract_metadata(hty_file_path);
    std::cout << "\nMetadata:\n" << meta.dump(4) << "\n";

    // Step 2: Project and display the "salary" column
    std::cout << "\nProjected Column: salary\n";
    std::vector<int> salary_column = project_single_column(meta, hty_file_path, "salary");
    display_column(meta, "salary", salary_column);

    // Step 3: Filter "salary > 30000"
    std::cout << "\nFiltered Column: salary > 30000\n";
    std::vector<int> filtered_salary = filter(meta, hty_file_path, "salary", 0, *((int*)&(float){30000.0}));
    display_column(meta, "salary", filtered_salary);

    // Step 4: Project multiple columns: id and salary
    std::cout << "\nProjected Columns: id, salary\n";
    std::vector<std::vector<int>> multi_col = project(meta, hty_file_path, {"id", "salary"});
    display_result_set(meta, {"id", "salary"}, multi_col);

    // Step 5: Add new rows to HTY file
    std::cout << "\nAdding new rows to test_modified.hty\n";
    std::vector<std::vector<int>> new_rows = {
        {5, 6},                     // id
        {2, 3},                     // type
        {*((int*)&(float){99000.0}), *((int*)&(float){65000.0})}  // salary
    };
    add_row(meta, hty_file_path, modified_hty_path, new_rows);

    // Step 6: Re-parse metadata from modified file and display updated results
    std::cout << "\nRe-loading Metadata from test_modified.hty\n";
    json new_meta = extract_metadata(modified_hty_path);
    std::vector<std::vector<int>> updated = project(new_meta, modified_hty_path, {"id", "salary"});
    display_result_set(new_meta, {"id", "salary"}, updated);

    return 0;
}