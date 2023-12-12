#include <algorithm>
#include <cassert>
#include <cstdint>
#include <cstdio>
#include <functional>
#include <random>
#include <set>
#include <string>
#include <unordered_set>
#include <utility>
#include <vector>
#include "SketchPolymer.h"

#define PERCENTAGE 0.95
#define THRESHOLD 300000 			/** Unit: microsecond  */

struct Quintet
{
	int delay;			   	// Delay of the package (microsecond)
	uint32_t SrcIP; 		// Source IP address
	uint32_t DstIP; 		// Destination IP Address
	uint16_t SrcPort;     	// Source IP Port (16 bits)
	uint16_t DstPort;    	// Destination IP Port (16 bits)
	uint8_t Protocol;       // Protocol type
	
	Quintet () {}
	Quintet (int _delay, uint32_t _SrcIP, uint32_t _DstIP,
		uint16_t _SrcPort, uint16_t _DstPort, uint8_t _Protocol) :
		delay (_delay), SrcIP (_SrcIP), DstIP (_DstIP),
		SrcPort (_SrcPort), DstPort (_DstPort), Protocol (_Protocol) {}

	void vectorize (std::vector <uint8_t> &s) const {

		s.clear ();
		s.reserve (13);
		uint32_t IP;
		uint16_t port;

		IP = this->SrcIP;
		for (int i = 3; i >= 0; --i) {
			s.push_back ((uint8_t) (IP & 0xFF));
			IP >>= 8;
		}
		IP = this->DstIP;
		for (int i = 3; i >= 0; --i) {
			s.push_back ((uint8_t) (IP & 0xFF));
			IP >>= 8;
		}

		port = this->SrcPort;
		for (int i = 1; i >= 0; --i) {
			s.push_back ((uint8_t) (port & 0xFF));
			port = (uint16_t) (port >> 8u);
		}
		port = this->DstPort;
		for (int i = 1; i >= 0; --i) {
			s.push_back ((uint8_t) (port & 0xFF));
			port = (uint16_t) (port >> 8u);
		}

		s.push_back ((uint8_t) this->Protocol);

		return;
	} 
};

// struct Quintet_Hashable {
// 	uint8_t data [13];
// 	Quintet_Hashable (Quintet &q) {
// 		std::vector <uint8_t> s;
// 		q.vectorize (s);
// 		assert (s.size () == 13);
// 		memcpy (data, s.data (), s.size ());
// 	}
// };

namespace DataSyntax {

std::set <int> protocols;
std::FILE *file;
int count;

int init (const char *name) {
	count = 0;
	file = fopen (name, "rb");
	return file != NULL;
}

/**
 * Return the quintet and the delay of a package.
*/
Quintet getQuin () {
	Quintet quin;
	if (fread (&quin.SrcIP, 4, 1, file) < 1) {
		quin.delay = -1;
		fclose (file);
	}
	else {
		++count;
		assert (fread (&quin.DstIP, 4, 1, file));
		assert (fread (&quin.SrcPort, 2, 1, file)); 
		assert (fread (&quin.DstPort, 2, 1, file)); 
		assert (fread (&quin.Protocol, 1, 1, file)); 
		assert (fread (&quin.delay, 4, 1, file)); 
		protocols.insert (quin.Protocol).second;
	}
	return quin;
}

}

std::set <std::vector <uint8_t>> alerted;
std::vector <Quintet> quinvec;

int main (int argc, char **argv) {
	assert (PERCENTAGE >= 0.5);
	assert (PERCENTAGE < 1 - 1e-3);
	const char input_name [] = "../../../130000_delay.dat";
	char *output_name = NULL;
	if (argc >= 3) {
		output_name = argv [2];
	}
	DataSyntax::init (input_name);
	for (int i = 0; ; ++i) {
		Quintet quin (DataSyntax::getQuin ());
		/** All quintets have been read and processed. */
		if (quin.delay < 0) {
			break;
		}
		quinvec.push_back (quin);
	}
	FILE *file = fopen (output_name, "w");
	assert (file != NULL);
	uint32_t memory;
	if (argc >= 2) {
		memory = atoi (argv [1]);
	}
	else {
		memory = 3 * (1 << 15);
	}
	assert (memory > 0);
	int count_alert = 0;
	int test_cycles = 10;
	timespec time1, time2;
	timespec time3, time4;
	double mops = 0, insertmops = 0, querymops = 0;
	clock_gettime (CLOCK_MONOTONIC, &time3); 
	for (int i = 0; i < test_cycles; ++i) {
		clock_gettime (CLOCK_MONOTONIC, &time4);
		if (time4.tv_sec - time3.tv_sec > 60)
			break;
		printf ("Running: test_cycle %d\n", i);
		SketchPolymer<__uint128_t>* sketchpolymer = new SketchPolymer<__uint128_t> (memory);
		alerted.clear ();
		std::vector <uint8_t> key;
		long long insertns = 0;
		long long queryns = 0;
		int time = 0, sz = (int) quinvec.size ();
		for (time = 0; time < sz; ++time) {
			Quintet quin = quinvec [time];
			quin.vectorize (key);
			__uint128_t quin_id = 0;
			for (auto &v : key) {
				quin_id = (quin_id << 8) + v;
			}
			clock_gettime (CLOCK_MONOTONIC, &time1); 
			sketchpolymer->insert (quin_id, 0, quin.delay);
			clock_gettime (CLOCK_MONOTONIC, &time2);
			insertns += (long long) (time2.tv_sec - time1.tv_sec) * 1000000000LL + (time2.tv_nsec - time1.tv_nsec); 
			clock_gettime (CLOCK_MONOTONIC, &time1); 			
			uint64_t res = sketchpolymer->query (quin_id, PERCENTAGE);
			clock_gettime (CLOCK_MONOTONIC, &time2);
			queryns += (long long) (time2.tv_sec - time1.tv_sec) * 1000000000LL + (time2.tv_nsec - time1.tv_nsec); 
			if (i > 0 || alerted.find (key) != alerted.end ())
				continue;
			/** Abnormal. Alert and add it to ALERTED. */
			if (res > THRESHOLD) {
				alerted.insert (key);
				++count_alert;
				fprintf (file, "0x");
				for (auto &v : key)
					fprintf (file, "%02X", v);
				fprintf (file, " %u\n", time);
			}
		}
		double test_insert_mops = (double) 1000.0 * time / (double) insertns;
		double test_query_mops = (double) 1000.0 * time / (double) queryns;
		double test_mops = (double) 1000.0 * time / (double) (insertns + queryns);
		insertmops += (test_insert_mops - insertmops) / (i + 1);
		querymops += (test_query_mops - querymops) / (i + 1);
        mops += (test_mops - mops) / (i + 1);
		if (i == 0) {
			printf ("Times of alerting: %d\n", count_alert);
			printf ("Finished! Time = %d, Memory Usage: %u\n", time, memory);
			printf ("Insert MOPS: %.12lf\nQuery MOPS: %.12lf\nMOPS: %.12lf\n", insertmops, querymops, mops);
			fprintf (file, "%d %u %.12lf %.12lf %.12lf\n", count_alert, memory, insertmops, querymops, mops);
			fclose (file);
		}
		delete sketchpolymer;
	}
	return 0;
}