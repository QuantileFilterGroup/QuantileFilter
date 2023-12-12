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

#define SKETCH_WIDTH_NARROW (1 << 6)
#define SKETCH_WIDTH_WIDE ((1 << 9) - SKETCH_WIDTH_NARROW)
#define SKETCH_NUMBER 3
#define PERCENTAGE 0.95
#define THRESHOLD 10000 			/** Unit: microsecond  */
#define DELTA 10					/** Alert if sum exceeds DELTA * (SCALE + ASCEND_PROB + 1) */

const int16_t SCALE = (uint16_t) (PERCENTAGE / (1 - PERCENTAGE));
const double ASCEND_PROB = PERCENTAGE / (1 - PERCENTAGE) - SCALE;
const int BAR = (int) (DELTA * (SCALE + ASCEND_PROB + 1));

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

class Sketch {
	private:
		uint16_t sketch_small[SKETCH_NUMBER][SKETCH_WIDTH_WIDE];
		uint16_t sketch_large[SKETCH_NUMBER][SKETCH_WIDTH_NARROW];
		std::mt19937 rnd;
		std::vector <uint32_t> seeds[2];
		int count_alert;
		FILE *file;
	public:
		/**
		 * Return memory the sketch currently uses.
		 * Unit: byte
		*/
		size_t memoryUsage () {
			return sizeof (sketch_small) + sizeof (sketch_large) +
				sizeof (rnd) + sizeof (file) +
				sizeof (seeds) + sizeof (count_alert) +
				seeds[0].capacity () * sizeof (uint32_t) +
				seeds[1].capacity () * sizeof (uint32_t);
		}

		void init (int argc, char **argv, const char *file_name) {
			if (argc <= 1) {
				rnd.seed (19260817);
			}
			else {
				rnd.seed (atoi (argv[1]));
			}
			if (argc <= 2) {
				assert (file_name != NULL);
			}
			else {
				file_name = argv [2];
			}
			file = fopen (file_name, "w");
			assert (file != NULL);
			/** Generate 2 * SKETCH_NUMBER random seeds as different hash functions, */
			for (int j = 0; j < 2; ++j) {
				for (int i = 0; i < SKETCH_NUMBER; ++i) {
					seeds[j].push_back (rnd ());
				}
			}
			
			printf ("Sketch initialization succeed. Memory usage: %llu bytes.\n", memoryUsage ());
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

		/**
		 * Return a pair of <finger prints, delay>,
		 * where finger prints are generated with differnt seeds,
		 * and the number of seeds is SKETCH_NUMBER.
		 * Every single seed conrresponds to a single hash function.
		*/
		std::pair <std::vector <uint32_t>, int> getPair (const Quintet &quin) {
			assert (quin.delay >= 0);
			std::vector <uint32_t> fp;
			for (int j = 0; j < 2; ++j) {
				for (int i = 0; i < SKETCH_NUMBER; ++i) {
					uint32_t seed = seeds[j][i];
					fp.push_back (murmur3_32 (quin, seed));
				}
			}
			return std::make_pair (fp, quin.delay);
		}

		/**
		 * Report an abnormal quantile.
		*/
		void alert (const Quintet &quin, uint32_t alert_time) {
			std::vector <uint8_t> s;
			quin.vectorize (s);

			assert (s.size () == 13);

			++count_alert;
			fprintf (file, "0x");
			for (auto &v : s)
				fprintf (file, "%02X", v);
			fprintf (file, " %u\n", alert_time);
		}

		/**
		 * Calculate a stream's summary with its finger prints.
		 * In this raw version, we use CM sketch, that is, to return
		 * the minimum of all positions of the stream as the summary.
		*/
		uint16_t calculate_large (const std::vector <uint32_t> &fp) {
			uint16_t minimum = 0xFFFF;
			for (int i = 0; i < SKETCH_NUMBER; ++i) {
				minimum = std::min (minimum, sketch_large[i][fp[i] % SKETCH_WIDTH_NARROW]);
			}
			return minimum;
		}

		uint16_t calculate_small (const std::vector <uint32_t> &fp) {
			uint16_t minimum = 0xFFFF;
			for (int i = 0; i < SKETCH_NUMBER; ++i) {
				minimum = std::min (minimum, sketch_small[i][fp[i] % SKETCH_WIDTH_WIDE]);
			}
			return minimum;
		}

		/**
		 * Insert a quintet into the sketch.
		 * Check if the stream should be reported right after insertion.
		 * If so, the stream's effect will be removed from the sketch.
		*/
		void insert (const Quintet &quin, uint32_t current_time) {
			auto pair = getPair (quin);
			assert (pair.first.size () == SKETCH_NUMBER * 2);
			std::vector <uint32_t> fp_small, fp_large;
			for (int i = 0; i < SKETCH_NUMBER; ++i) {
				fp_small.push_back (pair.first[i]);
				fp_large.push_back (pair.first[i + SKETCH_NUMBER]);
			}
			if (pair.second > THRESHOLD) {
				uint16_t new_val = (uint16_t) (calculate_large (fp_large) + 1);
				for (int i = 0; i < SKETCH_NUMBER; ++i) {
					int j = fp_large[i] % SKETCH_WIDTH_NARROW;
					sketch_large[i][j] = std::max (sketch_large[i][j], new_val);
				}
			}
			else {
				uint16_t new_val = (uint16_t) (calculate_small (fp_small) + 1);
				for (int i = 0; i < SKETCH_NUMBER; ++i) {
					int j = fp_small[i] % SKETCH_WIDTH_WIDE;
					sketch_small[i][j] = std::max (sketch_small[i][j], new_val);
				}
			}
			
			uint16_t large = calculate_large (fp_large);
			uint16_t small = calculate_small (fp_small);
			double ret = (SCALE + ASCEND_PROB) * large - small;
			if (ret > BAR) {
				alert (quin, current_time);
				/** Remove the effect of the stream. */
				for (int i = 0; i < SKETCH_NUMBER; ++i) {
					int j = fp_large[i] % SKETCH_WIDTH_NARROW;
					sketch_large[i][j] = (uint16_t) (sketch_large[i][j] - large);
				}
				for (int i = 0; i < SKETCH_NUMBER; ++i) {
					int j = fp_small[i] % SKETCH_WIDTH_WIDE;
					sketch_small[i][j] = (uint16_t) (sketch_small[i][j] - small);
				}
			}
		}

		void finish () {
			printf ("Times of alerting: %d\n", count_alert);
			printf ("Final memory usage: %llu bytes.", memoryUsage ());
			fprintf (file, "%d %llu\n", count_alert, memoryUsage ());
			fclose (file);
			return;
		}
} sketch;

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

int main (int argc, char **argv) {
	assert (PERCENTAGE >= 0.5);
	assert (PERCENTAGE < 1 - 1e-3);
	const char input_name [] = "../../../yahoo_delay.dat";
	char *output_name = NULL;
	DataSyntax::init (input_name);
	sketch.init (argc, argv, output_name);
	int time = 0;
	for (time = 0; ; ++time) {
		Quintet quin (DataSyntax::getQuin ());
		/** All quintets have been read and processed. */
		if (quin.delay < 0) {
			break;
		}
		sketch.insert (quin, time);
	}
	printf ("Finished! Time = %d\n", time);
	sketch.finish ();
	return 0;
}