#include <iostream>
#include "json.hpp"git commit -m "first commit"git push -u origin main
#include <fstream>
#include <vector>
#include <string>
#include <map>
#include <algorithm>
#include <cmath>
#include <nlohmann/json.hpp>
#include <boost/multiprecision/cpp_int.hpp>

using namespace std;
using namespace boost::multiprecision;
using json = nlohmann::json;

struct Share {
    int x;
    cpp_int y;
};

cpp_int decode_value(const string& value_str, const string& base_str) {
    int base = stoi(base_str);
    cpp_int value = 0;
    for (char c : value_str) {
        int digit;
        if (c >= '0' && c <= '9') {
            digit = c - '0';
        } else if (c >= 'a' && c <= 'z') {
            digit = c - 'a' + 10;
        } else if (c >= 'A' && c <= 'Z') {
            digit = c - 'A' + 10;
        } else {
            throw invalid_argument("Invalid character '" + string(1, c) + "' in value string");
        }

        if (digit >= base) {
            throw invalid_argument("Digit " + to_string(digit) + " exceeds base " + to_string(base));
        }

        value = value * base + digit;
    }
    return value;
}

vector<vector<Share>> generate_combinations(const vector<Share>& shares, int k) {
    vector<vector<Share>> combinations;
    vector<bool> mask(shares.size(), false);
    fill(mask.begin(), mask.begin() + k, true);
    do {
        vector<Share> combo;
        for (int i = 0; i < shares.size(); ++i) {
            if (mask[i]) {
                combo.push_back(shares[i]);
            }
        }
        combinations.push_back(combo);
    } while (prev_permutation(mask.begin(), mask.end()));
    return combinations;
}

cpp_int lagrange_interpolation(const vector<Share>& points) {
    cpp_int secret = 0;
    int k = points.size();

    for (int i = 0; i < k; ++i) {
        cpp_int xi = points[i].x;
        cpp_int yi = points[i].y;
        cpp_int numerator = 1;
        cpp_int denominator = 1;

        for (int j = 0; j < k; ++j) {
            if (i != j) {
                cpp_int xj = points[j].x;
                numerator *= -xj;
                denominator *= (xi - xj);
            }
        }

        if (denominator == 0) {
            throw runtime_error("Division by zero in Lagrange interpolation");
        }

        cpp_int term = yi * numerator / denominator;
        secret += term;
    }

    return secret;
}

cpp_int find_secret(const json& json_data) {
    if (!json_data.contains("keys") || !json_data["keys"].contains("n") || !json_data["keys"].contains("k")) {
        throw invalid_argument("Invalid JSON structure: missing 'keys', 'n', or 'k'");
    }

    int n = json_data["keys"]["n"];
    int k = json_data["keys"]["k"];
    vector<Share> shares;

    for (auto& item : json_data.items()) {
        if (item.key() == "keys") continue;

        if (!item.value().contains("base") || !item.value().contains("value")) {
            cerr << "Warning: Share " << item.key() << " is missing 'base' or 'value'. Skipping." << endl;
            continue;
        }

        try {
            int x = stoi(item.key());
            string base_str = item.value()["base"];
            string value_str = item.value()["value"];
            cpp_int y = decode_value(value_str, base_str);
            shares.push_back({x, y});
        } catch (const exception& e) {
            cerr << "Warning: Error processing share " << item.key() << ": " << e.what() << ". Skipping." << endl;
            continue;
        }
    }

    if (shares.size() < k) {
        throw runtime_error("Not enough valid shares to recover the secret");
    }

    auto combinations = generate_combinations(shares, k);
    map<cpp_int, int> frequency;

    for (const auto& combo : combinations) {
        try {
            cpp_int secret = lagrange_interpolation(combo);
            frequency[secret]++;
        } catch (const exception& e) {
            cerr << "Warning: Error in interpolation: " << e.what() << ". Skipping combination." << endl;
            continue;
        }
    }

    if (frequency.empty()) {
        throw runtime_error("No valid combinations found");
    }

    cpp_int best_secret = -1;
    int max_count = 0;
    for (const auto& [val, count] : frequency) {
        if (count > max_count) {
            max_count = count;
            best_secret = val;
        }
    }

    return best_secret;
}

int main() {
    try {
        ifstream file1("test_case1.json");
        if (!file1.is_open()) {
            cerr << "Error: Could not open test_case1.json" << endl;
            return 1;
        }
        json test_case1;
        file1 >> test_case1;
        cpp_int secret1 = find_secret(test_case1);
        cout << "Secret from Test Case 1: " << secret1 << endl;

        ifstream file2("test_case2.json");
        if (!file2.is_open()) {
            cerr << "Error: Could not open test_case2.json" << endl;
            return 1;
        }
        json test_case2;
        file2 >> test_case2;
        cpp_int secret2 = find_secret(test_case2);
        cout << "Secret from Test Case 2: " << secret2 << endl;
    } catch (const exception& e) {
        cerr << "Error: " << e.what() << endl;
        return 1;
    }

    return 0;
}