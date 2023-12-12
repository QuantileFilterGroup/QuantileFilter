#include <cassert>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <set>
#include <string>
#include <unordered_map>

#define THRESHOLD 10000 

std::unordered_map <std::string, double> my_map;
std::unordered_map <std::string, int> my_count;
std::set<std::string> output;

int T0 = 30;
int T1 = 50;

int main () {
	char in_name[] = "130000.dat";
	char out_name[] = "130000_delay.dat";
	std::FILE *in_file = fopen (in_name, "rb");
	std::FILE *out_file = fopen (out_name, "wb");
	// uint32_t SrcIP, DstIP;
	// uint16_t SrcPort, DstPort;
	// char Protocal;
	char buf[20];
	double timestamp;
	int count = 0;
	int out_count = 0;
	int high_delay = 0;
	uint32_t min_delay = 1e9, max_delay = 0;
	memset (buf, 0, sizeof (buf));
	for (bool flag = true; flag; ) {
		if (fread (buf, 1, 13, in_file) < 13) {
			flag = false;
		}
		else {
			++count;
			assert (fread (&timestamp, 8, 1, in_file));
			std::string s (buf, 13);
			auto it = my_map.find (s);
			if (it == my_map.end ()) {
				my_map[s] = timestamp;
				my_count[s] = 1;
			}
			else {
				output.insert (s);
				++out_count;
				fwrite (buf, 1, 13, out_file);
				uint32_t delay = (uint32_t) ((timestamp - it->second) * 1e6);
				if (delay > THRESHOLD)
					++high_delay;
				min_delay = std::min (min_delay, delay);
				max_delay = std::max (max_delay, delay);
				fwrite (&delay, 4, 1, out_file);
				it->second = timestamp;
				my_count[s] = my_count[s] + 1;
			}
		}
	}
	fclose (in_file);
	fclose (out_file);
	int no_less_than_T0 = 0, no_less_than_T1 = 1;
	int sum_no_less_than_T0 = 0, sum_no_less_than_T1 = 1;
	for (auto &v : my_count) {
		if (v.second >= T0) {
			++no_less_than_T0;
			sum_no_less_than_T0 += v.second;
		}
		if (v.second >= T1) {
			++no_less_than_T1;
			sum_no_less_than_T1 += v.second;
		}
	}
	printf ("Syntax succeeded: %d elements processed, %d elements output, %llu different streams detected.\n",
		count, out_count, output.size ());
	printf ("%d streams contains no less than %d elements.\n", no_less_than_T0, T0);
	printf ("%d streams contains no less than %d elements.\n", no_less_than_T1, T1);
	printf ("%d elements belong to a stream with no less than %d elements.\n", sum_no_less_than_T0, T0);
	printf ("%d elements belong to a stream with no less than %d elements.\n", sum_no_less_than_T1, T1);
	printf ("%d elements are high-delayed.\n", high_delay);
	printf ("Minimum delay is %d, maximum delay is %d.\n", min_delay, max_delay);
	return 0;
}