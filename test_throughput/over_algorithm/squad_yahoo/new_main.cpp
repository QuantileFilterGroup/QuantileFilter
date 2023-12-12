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
#include "lossycountwSgk.h"

#define PERCENTAGE 0.95
#define THRESHOLD 20000000 			/** Unit: microsecond  */

const int16_t SCALE = (uint16_t) (PERCENTAGE / (1 - PERCENTAGE));
const double ASCEND_PROB = PERCENTAGE / (1 - PERCENTAGE) - SCALE;

struct Quintet
{
	int delay;			   	// 这个包的延迟（微秒）
	uint32_t SrcIP; 		// 源IP地址
	uint32_t DstIP; 		// 目的IP地址
	uint16_t SrcPort;     	// 源端口号16bit
	uint16_t DstPort;    	// 目的端口号16bit
	uint8_t Protocol;       // 协议类型
	
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

namespace MyHash {

std::mt19937 rnd;
uint32_t seed1, seed2;

void init (uint32_t _seed) {
	rnd.seed (_seed);
	seed1 = rnd ();
	seed2 = rnd ();
	return;
}

uint32_t murmur_32_scramble (uint32_t k) {
	k *= 0xcc9e2d51;
	k = (k << 15) | (k >> 17);
	k *= 0x1b873593;
	return k;
}

/**
 * Slightly modified version of murmur3 hash.
 * Considering the Quintet as a 20-byte (5 elements * 4 bytes) long array.
*/
uint32_t murmur3_32 (const Quintet &key, uint32_t seed)
{
	uint32_t h = seed;
	uint32_t k;

	uint32_t count_len = 0;

	k = key.SrcIP;
	h ^= murmur_32_scramble(k);
	h = (h << 13) | (h >> 19);
	h = h * 5 + 0xe6546b64;
	++count_len;

	k = key.DstIP;
	h ^= murmur_32_scramble(k);
	h = (h << 13) | (h >> 19);
	h = h * 5 + 0xe6546b64;
	++count_len;

	k = key.SrcPort;
	h ^= murmur_32_scramble(k);
	h = (h << 13) | (h >> 19);
	h = h * 5 + 0xe6546b64;
	++count_len;

	k = key.DstPort;
	h ^= murmur_32_scramble(k);
	h = (h << 13) | (h >> 19);
	h = h * 5 + 0xe6546b64;
	++count_len;

	k = key.Protocol;
	h ^= murmur_32_scramble(k);
	h = (h << 13) | (h >> 19);
	h = h * 5 + 0xe6546b64;
	++count_len;

	count_len *= (uint32_t) sizeof (uint32_t);

	/* Finalize. */
	h ^= count_len;
	h ^= h >> 16;
	h *= 0x85ebca6b;
	h ^= h >> 13;
	h *= 0xc2b2ae35;
	h ^= h >> 16;
	return h;
}

uint64_t murmur3_64 (const Quintet &key) {
	return (((uint64_t) murmur3_32 (key, seed1)) << 32 | ((uint64_t) murmur3_32 (key, seed2)));
}

}

std::set <std::vector <uint8_t>> alerted;
std::vector <Quintet> quinvec;

int main (int argc, char **argv) {
	assert (PERCENTAGE >= 0.5);
	assert (PERCENTAGE < 1 - 1e-3);
	const char input_name [] = "../../../yahoo_delay.dat";
	char *output_name = NULL;
	DataSyntax::init (input_name);
	for (int i = 0; ; ++i) {
		Quintet quin (DataSyntax::getQuin ());
		/** All quintets have been read and processed. */
		if (quin.delay < 0) {
			break;
		}
		quinvec.push_back (quin);
	}
	/** Only streams no less than this size can be recorded. */
	const int SKETCH_THRESHOLD = 50;
	int dataset_length = 0;
	double epsilon = 0;
	if (argc < 4) {
		printf ("Usage: new_main dataset_length epsilon file_name [hash_seed]\n");
		return 0;
	}
	dataset_length = atoi (argv [1]);
	epsilon = std::stod (argv [2]);
	output_name = argv [3];
	FILE *file = fopen (output_name, "w");
	assert (file != NULL);
	assert (dataset_length > 0);
	assert (epsilon > 0);
	if (argc >= 5) {
		MyHash::init (atoi (argv [4]));
	}
	else {
		/** Default hash seed. */
		MyHash::init (19260817);
	}
	double theta = (double) SKETCH_THRESHOLD / dataset_length;
	printf ("Theta: %lf, Scale: %lf\n", theta, std::pow (theta, -1));
	long long internal_counters = (long long) (std::pow (theta, -1) * std::pow (epsilon, -0.5));
	// We are using QUASI, so this is useless, but anyway, they didn't count this in memory consumption...
	long long samples_num = 4 * (long long) (std::pow (theta, -2) * std::pow (epsilon, -2) * std::pow ((double) internal_counters, -1));
	double qsketch_eps = epsilon / 2.0;
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
		LCU_type* squad = LCU_Init (std::pow ((double) internal_counters, -1), qsketch_eps, samples_num);
		alerted.clear ();
		std::vector <uint8_t> key;
		long long insertns = 0;
		long long queryns = 0;
		int time = 0, sz = (int) quinvec.size ();
		for (time = 0; time < sz; ++time) {
			Quintet quin = quinvec [time];
			quin.vectorize (key);
			uint64_t quin_id = MyHash::murmur3_64 (quin);
			clock_gettime (CLOCK_MONOTONIC, &time1); 
			LCU_UpdateLatency (squad, quin_id, (long double) quin.delay);
			clock_gettime (CLOCK_MONOTONIC, &time2);
			insertns += (long long) (time2.tv_sec - time1.tv_sec) * 1000000000LL + (time2.tv_nsec - time1.tv_nsec); 
			clock_gettime (CLOCK_MONOTONIC, &time1); 			
			long double res = LCU_QuantileQuery (squad, quin_id, PERCENTAGE);
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
			uint32_t memory = (uint32_t) LCU_Size (squad);
			printf ("Times of alerting: %d\n", count_alert);
			printf ("Finished! Time = %d, Memory Usage: %u\n", time, memory);
			printf ("Insert MOPS: %.12lf\nQuery MOPS: %.12lf\nMOPS: %.12lf\n", insertmops, querymops, mops);
			fprintf (file, "%d %u %.12lf %.12lf %.12lf\n", count_alert, memory, insertmops, querymops, mops);
			fclose (file);
		}
		delete squad;
	}
	return 0;
}