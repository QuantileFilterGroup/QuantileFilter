#include <cassert>
#include <cstdint>
#include <cstdio>
#include <queue>
#include <set>
#include <map>
#include <utility>
#include <vector>

#define PERCENTAGE 0.95
#define THRESHOLD 300000 			/** Unit: microsecond  */
#define DELTA 20
#define EPSILON 1e-7				/** Used for ceiling */

std::map <std::vector <uint8_t>, std::pair <std::priority_queue <int>,
	std::priority_queue <int, std::vector <int>, std::greater <int>>>> record;

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

int main () {
	assert (PERCENTAGE >= 0.5);
	assert (PERCENTAGE < 1 - 1e-3);
	const char input_name[] = "zipf_3.dat";
	const char output_name[] = "answer_zipf_3.out";
	DataSyntax::init (input_name);
	FILE *file = fopen (output_name, "w");
	int count_alert = 0;
	int time = 0;
	std::vector <uint8_t> s;
	for (time = 0; ; ++time) {
		Quintet quin (DataSyntax::getQuin ());
		/** All quintets have been read and processed. */
		if (quin.delay < 0) {
			break;
		}
		quin.vectorize (s);
		auto it = record.find (s);
		if (it == record.end ()) {
			it = record.insert (std::make_pair (s, std::make_pair (std::priority_queue <int> (),
				std::priority_queue <int, std::vector <int>, std::greater <int>> ()))).first;
		}
		std::priority_queue <int> *smaller = &it->second.first;
		std::priority_queue <int, std::vector <int>, std::greater <int>> *larger = &it->second.second;
		if (larger->size () == 0 || quin.delay >= larger->top ()) {
			larger->push (quin.delay);
		}
		else {
			smaller->push (quin.delay);
		}
		int total = (int) (smaller->size () + larger->size ());
		size_t quantile = (size_t) ((1 - PERCENTAGE) * total + (1 - EPSILON) + DELTA);
		while (larger->size () < quantile && smaller->size () > 0) {
			larger->push (smaller->top ());
			smaller->pop ();
		}
		while (larger->size () > quantile) {
			smaller->push (larger->top ());
			larger->pop ();
		}
		if (larger->size () == quantile && larger->top () > THRESHOLD) {
			++count_alert;
			fprintf (file, "0x");
			for (auto &v : s)
				fprintf (file, "%02X", v);
			fprintf (file, " %u\n", time);
			while (larger->size ()) {
				larger->pop ();
			}
			while (smaller->size ()) {
				smaller->pop ();
			}
		}
	}
	printf ("Finished! Time = %d\n", time);
	printf ("Times of alerting: %d\n", count_alert);
	size_t memory_assess = sizeof (record) + time * sizeof (int) + record.size () *
		(sizeof (std::vector <uint8_t>) + 2 * sizeof (std::priority_queue <int>) + 13 * sizeof (uint8_t));
	printf ("Final memory usage: %llu bytes.", memory_assess);
	fclose (file);
}