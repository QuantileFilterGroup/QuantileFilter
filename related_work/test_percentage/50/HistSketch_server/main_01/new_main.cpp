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
#include "sketch.h"

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

sketch<uint32_t, H_BUCKET_NUM, SLOT_NUM, CM_DEPTH_HIST, CM_WIDTH_HIST, BLOOM_SIZE, BLOOM_HASH_NUM> hist;
std::set <std::vector <uint8_t>> alerted;

int main (int argc, char **argv) {
	assert (PERCENTAGE >= 0.5);
	assert (PERCENTAGE < 1 - 1e-3);
	const char input_name [] = "../../../130000_delay.dat";
	char *output_name = NULL;
	if (argc >= 2) {
		output_name = argv [1];
	}
	DataSyntax::init (input_name);
	FILE *file = fopen (output_name, "w");
	assert (file != NULL);
	std::vector <uint8_t> key;
	int count_alert = 0;
	int time = 0;
	for (time = 0; ; ++time) {
		Quintet quin (DataSyntax::getQuin ());
		/** All quintets have been read and processed. */
		if (quin.delay < 0) {
			break;
		}
		quin.vectorize (key);
		uint8_t bid = quin.delay > THRESHOLD;
		hist.insert ((Key_t) (&(*key.begin ())), bid);
		/**
		 * Problem 1: Original Hist Sketch doesn't support deletion.
		 * 			If we only care about F1 Score, we can only trigger
		 * 			alertion the first time quantile is abnormal.
		*/
		if (alerted.find (key) != alerted.end ())
			continue;
		Hist_t result = hist.histogramQuery ((Key_t) (&(*key.begin ())));
		int32_t small_equ = (int32_t) result.at (0);
		int32_t large = (int32_t) result.at (1);
		/** Abnormal. Alert and add it to ALERTED. */
		if (small_equ <= (int32_t) ((small_equ + large) * PERCENTAGE - DELTA)) {
			alerted.insert (key);
			++count_alert;
			fprintf (file, "0x");
			for (auto &v : key)
				fprintf (file, "%02X", v);
			fprintf (file, " %u\n", time);
		}
	}
	printf ("Times of alerting: %d\n", count_alert);
	size_t memory = hist.get_memory_usage ();
	printf ("Finished! Time = %d, Memory Usage: %llu\n", time, memory);
	fprintf (file, "%d %llu\n", count_alert, memory);
	fclose (file);
	return 0;
}




