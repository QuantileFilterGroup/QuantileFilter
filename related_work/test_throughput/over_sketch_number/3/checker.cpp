#include <cassert>
#include <cstdio>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <iterator>
#include <map>
#include <sstream>
#include <string>
#include <vector>

int n, m;
std::map <std::string, std::vector <int>> answer;
std::map <std::string, std::vector <int>> output;

std::string formatDouble (double number, int precision) {
    std::stringstream stream;
    stream << std::fixed << std::setprecision (precision) << number;
    std::string str = stream.str ();
    size_t dot_pos = str.find ('.');
    if (dot_pos != std::string::npos) {
		if ((int) dot_pos < precision) {
			str = str.substr (0, precision + 1);
		}
		else {
			str = str.substr (0, dot_pos);
		}
    }
    return str;
}

int main (int argc, char **argv) {
    std::string csv_path = "./summary.csv";

	if (argc < 3) {
		printf ("Usage: checker <csv_line> <output_file> [answer_file = answer_new_threshold.out] [n] [m]\n");
		return 0;
	}
	int line_num = atoi (argv[1]);
	FILE *file_out = fopen (argv[2], "r");
	if (file_out == NULL) {
		printf ("Cannot open output file!\n");
		return 0;
	}
	FILE *file_ans;
	if (argc >= 4) {
		file_ans = fopen (argv[3], "r");
		if (file_ans == NULL) {
			printf ("Cannot open answer file!\n");
			return 0;
		}
	}
	else {
		char name_ans[] = "../../../answer_new_threshold.out";
		file_ans = fopen (name_ans, "r");
		if (file_ans == NULL) {
			printf ("Cannot open default answer file \"answer_new_threshold.out\"!\n");
			return 0;
		}
	}
	if (argc >= 5) {
		n = atoi (argv[4]);
	}
	else {
		/** Elements of 130000.dat */
		n = 26121161;
	}
	if (argc >= 6) {
		m = atoi (argv[5]);
	}
	else {
		/** Different streams of 130000.dat */
		m = 640577;
	}
	assert (file_out != NULL);
	assert (file_ans != NULL);

	char buf[30];
	int time_stamp;
	while (fscanf (file_ans, "%s", buf) != EOF) {
		std::string s(buf);
		assert (s.length () == 28);
		assert (fscanf (file_ans, "%d", &time_stamp) == 1);
		auto it = answer.find (s);
		if (it == answer.end ()) {
			answer.insert (std::make_pair (s, std::vector <int> (1, time_stamp)));
		}
		else {
			it->second.push_back (time_stamp);
		}
	}

	int memory = 0;
	int alert_count = 0;
	double mops = 0;
	while (fscanf (file_out, "%s", buf) != EOF) {
		std::string s(buf);
		if (s.length () != 28) {
			assert (fscanf (file_out, "%d", &memory) == 1);
			assert (fscanf (file_out, "%lf", &mops) == 1);
			assert (fscanf (file_out, "%s", buf) == EOF);
			alert_count = atoi (s.c_str ());
			break;
		}
		assert (fscanf (file_out, "%d", &time_stamp) == 1);
		auto it = output.find (s);
		if (it == output.end ()) {
			output.insert (std::make_pair (s, std::vector <int> (1, time_stamp)));
		}
		else {
			it->second.push_back (time_stamp);
		}
	}

	int TP = 0, FN = 0, FP = 0, TP_dummy = 0;
	long long sum_abs_delta = 0;
	for (auto &v : output) {
		auto it = answer.find (v.first);
		if (it != answer.end ()) {
			++TP;
			size_t len = (int) std::min (v.second.size (), it->second.size ());
			for (size_t i = 0; i < len; ++i)
				sum_abs_delta += std::abs (v.second[i] - it->second[i]);
			for (size_t i = len; i < v.second.size (); ++i)
				sum_abs_delta += std::abs (n + 1 - v.second[i]);
			for (size_t i = len; i < it->second.size (); ++i)
				sum_abs_delta += std::abs (n + 1 - it->second[i]);
		}
		else {
			++FP;
			for (auto &u : v.second)
				sum_abs_delta += n + 1 - u;
		}
	}
	for (auto &v : answer) {
		auto it = output.find (v.first);
		if (it != output.end ()) {
			++TP_dummy;
		}
		else {
			++FN;
			for (auto &u : v.second)
				sum_abs_delta += n + 1 - u;
		}
	}
	assert (TP_dummy == TP);
	int TN = m - TP - FP - FN;
	double P = (double) TP / (TP + FP) * 100, R = (double) TP / (TP + FN) * 100;

	std::ifstream existing_csvFile (csv_path);
    std::vector<std::string> lines;
    std::string line;
    while (std::getline (existing_csvFile, line)) {
        lines.push_back (line);
    }
    existing_csvFile.close ();

    std::stringstream ss;
	ss << memory << ","
		<< alert_count << ","
		<< TP << ","
		<< FP << ","
		<< FN << ","
		<< TN << ","
		<< formatDouble (2 * P * R / (P + R), 8) << ","
		<< formatDouble ((double) (TP + TN) / m * 100, 6) << ","
		<< formatDouble (P, 6) << ","
		<< formatDouble (R, 6) << ","
		<< sum_abs_delta << ","
		<< formatDouble (mops, 12);
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