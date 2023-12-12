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

int main (int argc, char **argv) {
	assert (PERCENTAGE >= 0.5);
	assert (PERCENTAGE < 1 - 1e-3);
	const char input_name[] = "../../../zipf_4.dat";
	char *output_name = NULL;
	if (argc >= 3) {
		output_name = argv [2];
	}
	DataSyntax::init (input_name);
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
	SketchPolymer<__uint128_t>* sketchpolymer = new SketchPolymer<__uint128_t> (memory);
	int count_alert = 0;
	int time = 0;
	for (time = 0; ; ++time) {
		Quintet quin (DataSyntax::getQuin ());
		/** All quintets have been read and processed. */
		if (quin.delay < 0) {
			break;
		}
		// Quintet_Hashable quin_hash (quin);
		std::vector <uint8_t> s;
		quin.vectorize (s);
		__uint128_t quin_id = 0;
		for (auto &v : s) {
			quin_id = (quin_id << 8) + v;
		}
		sketchpolymer->insert (quin_id, 0, quin.delay);
		/**
		 * Problem 1: Original Sketch Polymer doesn't support deletion.
		 * 			If we only care about F1 Score, we can only trigger
		 * 			alertion the first time quantile is abnormal.
		*/
		if (alerted.find (s) != alerted.end ())
			continue;
		/**
		 * Problem 2: Original Sketch Polymer doesn't support DELTA.
		 * 			It can only answer what is exactly the quantile.
		 * 			Here we just ignore the DELTA. Note that answer
		 * 			considers about DELTA, so there will be system error.
		*/
		if (sketchpolymer->query (quin_id, PERCENTAGE) > THRESHOLD) {
			alerted.insert (s);
			++count_alert;
			fprintf (file, "0x");
			for (auto &v : s)
				fprintf (file, "%02X", v);
			fprintf (file, " %u\n", time);
		}
	}
	printf ("Times of alerting: %d\n", count_alert);
	printf ("Finished! Time = %d\n", time);
	fprintf (file, "%d %llu\n", count_alert, memory);
	fclose (file);
	return 0;
}