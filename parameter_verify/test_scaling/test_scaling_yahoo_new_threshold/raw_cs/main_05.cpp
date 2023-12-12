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

#define SKETCH_WIDTH (1 << 13)
#define SKETCH_NUMBER 3
#define PERCENTAGE 0.95
#define THRESHOLD 20000000 			/** Unit: microsecond  */
#define DELTA 10					/** Alert if sum exceeds DELTA * (SCALE + ASCEND_PROB + 1) */
#define MIN_VAL ((int16_t) 0x8000)
#define MAX_VAL ((int16_t) 0x7FFF)

const int16_t SCALE = (uint16_t) (PERCENTAGE / (1 - PERCENTAGE));
const double ASCEND_PROB = PERCENTAGE / (1 - PERCENTAGE) - SCALE;
const int BAR = (int) (DELTA * (SCALE + ASCEND_PROB + 1));

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

class Sketch {
	private:
		int16_t sketch[SKETCH_NUMBER][SKETCH_WIDTH];
		std::mt19937 rnd;
		std::vector <uint32_t> seeds;
		int count_alert;
		FILE *file;
	public:
		/**
		 * Return memory the sketch currently uses.
		 * Unit: byte
		*/
		size_t memoryUsage () {
			return sizeof (sketch) + sizeof (rnd) + sizeof (file) +
				sizeof (seeds) + sizeof (count_alert) +
				seeds.capacity () * sizeof (uint32_t);
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
			/** Generate SKETCH_NUMBER random seeds as different hash functions.
			 * One more seed for generating positive/negative.
			 */
			for (int i = 0; i < SKETCH_NUMBER + 1; ++i) {
				seeds.push_back (rnd ());
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
			for (int i = 0; i < SKETCH_NUMBER + 1; ++i) {
				uint32_t seed = seeds[i];
				fp.push_back (murmur3_32 (quin, seed));
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
		 * In this raw version, we use Count Sketch, that is, to return
		 * the median of all positions of the stream as the summary.
		*/
		int16_t calculate (const std::vector <uint32_t> &fp) {
			std::vector <int32_t> numbers;
			for (int i = 0; i < SKETCH_NUMBER; ++i) {
				numbers.push_back ((int32_t) sketch[i][fp[i] % SKETCH_WIDTH]);
			}
			std::sort (numbers.begin (), numbers.end ());
			int32_t ret = (numbers[SKETCH_NUMBER / 2] + numbers[(SKETCH_NUMBER - 1) / 2]) / 2;
			return (int16_t) ret;
		}

		/**
		 * A protection that a plus operation for 16-bit int won't get overflow.
		*/
		int16_t protectedPlus (int16_t originalValue, int16_t delta) {
			int16_t newValue = (int16_t) (originalValue + delta);
			if (delta < 0 && newValue > originalValue) {
				return MIN_VAL;
			}
			if (delta > 0 && newValue < originalValue) {
				return MAX_VAL;
			}
			return newValue;
		}

		/**
		 * Insert a quintet into the sketch.
		 * Check if the stream should be reported right after insertion.
		 * If so, the stream's effect will be removed from the sketch.
		*/
		void insert (const Quintet &quin, uint32_t current_time) {
			auto pair = getPair (quin);
			assert (pair.first.size () == SKETCH_NUMBER + 1);
			int16_t sign = (pair.first[SKETCH_NUMBER] & 1) ? -1 : 1;
			for (int i = 0; i < SKETCH_NUMBER; ++i) {
				int j = pair.first[i] % SKETCH_WIDTH;
				if (pair.second > THRESHOLD) {
					sketch[i][j] = protectedPlus (sketch[i][j], (int16_t) (SCALE * sign));
					if (rnd () <= ASCEND_PROB * rnd.max ()) {
						sketch[i][j] = protectedPlus (sketch[i][j], sign);
					}
				}
				else {
					sketch[i][j] = protectedPlus (sketch[i][j], (int16_t) -sign);
				}
			}
			int16_t ret = calculate (pair.first);
			if (ret * sign > BAR) {
				alert (quin, current_time);
				/** Remove the effect of the stream. */
				for (int i = 0; i < SKETCH_NUMBER; ++i) {
					int j = pair.first[i] % SKETCH_WIDTH;
					sketch[i][j] = protectedPlus (sketch[i][j], (int16_t) -ret);
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