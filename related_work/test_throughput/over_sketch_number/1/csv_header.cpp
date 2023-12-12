#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <fstream>
#include <iostream>
#include <iterator>
#include <sstream>
#include <string>
#include <vector>

int main () {
    std::string csv_path = "./summary.csv";
    int line_num = 0;

    std::ifstream existing_csvFile (csv_path);
    std::vector<std::string> lines;
    std::string line;
    while (std::getline (existing_csvFile, line)) {
        lines.push_back (line);
    }
    existing_csvFile.close ();


    std::stringstream ss;
	ss << "Memory" << ","
        << "Alert" << ","
		<< "TP" << ","
		<< "FP" << ","
		<< "FN" << ","
		<< "TN" << ","
		<< "F1_Score" << ","
		<< "Accuracy" << ","
		<< "Precision" << ","
		<< "Recall" << ","
		<< "Sum_of_Absolute_Delta" << ","
        << "MOPS";
    std::string new_line = ss.str ();

    if (line_num >= 0 && line_num < (int) lines.size ()) {
        lines[line_num] = new_line;
    }
	else if (line_num == (int) lines.size ()) {
		lines.push_back (new_line);
	}
	else {
        std::cerr << "Invalid line number: " << line_num << std::endl;
        return 1;
    }

    std::ofstream csv_file (csv_path);
    if (csv_file.is_open ()) {
        std::copy (lines.begin (), lines.end (), std::ostream_iterator<std::string> (csv_file, "\n"));
        csv_file.close ();
    }
	else {
        std::cerr << "Failed to open CSV file: " << csv_path << std::endl;
        return 1;
    }

    return 0;
}
