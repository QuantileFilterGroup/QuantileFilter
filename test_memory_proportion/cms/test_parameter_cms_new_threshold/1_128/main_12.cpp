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

#define SKETCH_WIDTH (1 << 12)
#define SKETCH_NUMBER 3
#define PERCENTAGE 0.95
#define THRESHOLD 300000 			/** Unit: microsecond  */
#define DELTA 30					/** Alert if sum exceeds DELTA * (SCALE + ASCEND_PROB + 1) */
#define BLOCK_NUMBER_BITS 16
#define BLOCK_NUMBER (1 << BLOCK_NUMBER_BITS)
#define BLOCK_LENGTH 12
#define FP_MASK ((1 << 16) - 1)
#define TOTAL_FP_MASK ((1ll << (16 + BLOCK_NUMBER_BITS)) - 1)
#define EMPTY_BLOCK ((int16_t) 0x8000)
#define MIN_VAL ((int16_t) 0x8001)
#define MAX_VAL ((int16_t) 0x7FFF)

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
		int16_t sketch [SKETCH_NUMBER][SKETCH_WIDTH];
		uint16_t heavy_fp [BLOCK_NUMBER][BLOCK_LENGTH];
		int16_t heavy_value [BLOCK_NUMBER][BLOCK_LENGTH];
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
			return sizeof (sketch) + sizeof (heavy_fp) + sizeof (heavy_value) +
				sizeof (rnd) + sizeof (seeds) + sizeof (count_alert) +
				sizeof (file) +
				seeds.capacity () * sizeof (uint32_t);
		}

		void init (int argc, char **argv, const char *file_name) {
			if (argc <= 1) {
				rnd.seed (19260817);
			}
			else {
				rnd.seed (atoi (argv [1]));
			}
			if (argc <= 2) {
				assert (file_name != NULL);
			}
			else {
				file_name = argv [2];
			}
			file = fopen (file_name, "w");
			assert (file != NULL);
			/** Generate SKETCH_NUMBER random seeds as different hash functions, */
			for (int i = 0; i < SKETCH_NUMBER + 1; ++i) {
				seeds.push_back (rnd ());
			}
			for (int i = 0; i < BLOCK_NUMBER; ++i) {
				for (int j = 0; j < BLOCK_LENGTH; ++j) {
					heavy_value [i][j] = EMPTY_BLOCK;
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
		 * Calculate the hash value of a single 32-bit integer.
		*/
		uint32_t murmur3_32_single (const uint32_t &key, uint32_t seed) {
			uint32_t h = seed;
			uint32_t k;

			uint32_t count_len = 0;

			k = key;
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
		std::pair <uint32_t, int> getPair (const Quintet &quin) {
			assert (quin.delay >= 0);
			uint32_t long_fp = murmur3_32 (quin, seeds [0]) & TOTAL_FP_MASK;
			return std::make_pair (long_fp, quin.delay);
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
		 * Calculate the block index of a Quintet in the heavy part.
		*/
		inline uint32_t getBlockIndex (const uint32_t &fp) {
			return ((fp & TOTAL_FP_MASK) >> 16);
		}

		/**
		 * Return the <block index, in-block offset> of a finger print.
		 * Return <-1, -1> if not found.
		*/
		std::pair <int32_t, int32_t> findBlock (const uint32_t &fp) {
			uint32_t block_index = getBlockIndex (fp);
			uint16_t finger_print = fp & FP_MASK;
			for (int i = 0; i < BLOCK_LENGTH; ++i) {
				if (heavy_fp [block_index][i] == finger_print &&
					heavy_value [block_index][i] != EMPTY_BLOCK) {
						return std::make_pair ((int32_t) block_index, i);
					}
			}
			return std::make_pair (-1, -1);
		}

		/**
		 * Calculate finger prints for light part, using its
		 * full finger print.
		*/
		std::vector <uint32_t> getLightFPs (uint32_t fp) {
			std::vector <uint32_t> ret;
			for (int i = 1; i < SKETCH_NUMBER + 1; ++i) {
				ret.push_back (murmur3_32_single (fp, seeds [i]));
			}
			return ret;
		}

		/**
		 * Calculate a stream's summary with its finger prints.
		 * In this version, we use CM sketch, that is, to return
		 * the minimum of all positions of the stream as the summary.
		*/
		int16_t calculate (const uint32_t &fp) {
			auto pair = findBlock (fp);
			if (pair.first >= 0 && pair.second >= 0) {
				return heavy_value [pair.first][pair.second];
			}
			auto lightFPs = getLightFPs (fp);
			int16_t minimum = MAX_VAL;
			for (int i = 0; i < SKETCH_NUMBER; ++i) {
				minimum = std::min (minimum, sketch [i][lightFPs [i] % SKETCH_WIDTH]);
			}
			return minimum;
		}

		/**
		 * Calculate the full finger print with a block index and
		 * the short 16-bit finger print.
		 */
		uint32_t reviveFullFP (uint32_t index, uint16_t fp) {
			return ((index << 16) | fp) & TOTAL_FP_MASK;
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
		 * Insert a <finger print, value> pair into the light part.
		 * Provided that the finger print didn't trigger the alert
		 * in the heavy part, so we assume it's normal by now,
		 * thus we don't check alerting after the insertion.
		*/
		void insertLight (uint32_t fp, int16_t value) {
			std::vector <uint32_t> lightFPs = getLightFPs (fp);
			assert (lightFPs.size () == SKETCH_NUMBER);
			for (int i = 0; i < SKETCH_NUMBER; ++i) {
				int j = lightFPs [i] % SKETCH_WIDTH;
				sketch [i][j] = protectedPlus (sketch [i][j], value);
			}
			return;
		}

		/**
		 * Get the address of the <finger print, value> pair who got
		 * the minimum value in the heavy part, same block with `fp`.
		*/
		std::pair <int32_t, int32_t> getHeavyMin (uint32_t fp) {
			uint32_t block_index = getBlockIndex (fp);
			int argmin = 0;
			for (int i = 1; i < BLOCK_LENGTH; ++i) {
				if (heavy_value [block_index][i] < heavy_value [block_index][argmin])
					argmin = i;
			}
			return std::make_pair (block_index, argmin);
		}

		/**
		 * Insert a quintet into the sketch.
		 * Check if the stream should be reported right after insertion.
		 * If so, the stream's effect will be removed from the sketch.
		*/
		void insert (const Quintet &quin, uint32_t current_time) {
			auto pair = getPair (quin);
			auto ind = findBlock (pair.first);
			/** The stream exists in heavy part. */
			if (ind.first >= 0 && ind.second >= 0) {
				if (pair.second > THRESHOLD) {
					heavy_value [ind.first][ind.second] =
						protectedPlus (heavy_value [ind.first][ind.second], SCALE);
					if (rnd () <= ASCEND_PROB * rnd.max ()) {
						heavy_value [ind.first][ind.second] =
							protectedPlus (heavy_value [ind.first][ind.second], 1);
					}
				}
				else {
					heavy_value [ind.first][ind.second] =
						protectedPlus (heavy_value [ind.first][ind.second], -1);
				}
				if (heavy_value [ind.first][ind.second] > BAR) {
					alert (quin, current_time);
					heavy_value [ind.first][ind.second] = 0;
				}
				return;
			}

			/** The stream not exists in heavy part. */

			/**
			 * The stream is a delayed one. See whether we need to
			 * replace an element in heavy part or not.
			 */
			if (pair.second > THRESHOLD) {
				int16_t val = calculate (pair.first);
				int16_t new_val = val;
				new_val = protectedPlus (new_val, SCALE);
				if (rnd () <= ASCEND_PROB * rnd.max ()) {
					new_val = protectedPlus (new_val, 1);
				}
				/** Value of the stream is large enough for alerting. */
				if (new_val > BAR) {
					/** Remove the effect of the stream in light part. */
					alert (quin, current_time);
					auto LightFPs = getLightFPs (pair.first);
					for (int i = 0; i < SKETCH_NUMBER; ++i) {
						int j = LightFPs [i] % SKETCH_WIDTH;
						sketch [i][j] = protectedPlus (sketch [i][j], (int16_t) -val);
					}
					return;
				}
				/**
				 * Not large enough. Replace the minimum of
				 * the heavy part if the new value is larger than it.
				 */
				auto argmin = getHeavyMin (pair.first);
				int16_t tmp_val = heavy_value [argmin.first][argmin.second];
				/** Not replacing. Just let new value go back. */
				if (tmp_val >= new_val) {
					int16_t delta = protectedPlus (new_val, (int16_t) -val);
					auto LightFPs = getLightFPs (pair.first);
					for (int i = 0; i < SKETCH_NUMBER; ++i) {
						int j = LightFPs [i] % SKETCH_WIDTH;
						sketch [i][j] = protectedPlus (sketch [i][j], delta);
					}
					return;
				}
				/** Replacing. Add the old heavy one to light part. */
				if (tmp_val != EMPTY_BLOCK) {
					insertLight (reviveFullFP (argmin.first, heavy_fp [argmin.first][argmin.second]),
						tmp_val);
				}
				auto LightFPs = getLightFPs (pair.first);
				for (int i = 0; i < SKETCH_NUMBER; ++i) {
					int j = LightFPs [i] % SKETCH_WIDTH;
					sketch [i][j] = protectedPlus (sketch [i][j], (int16_t) -val);
				}
				heavy_fp [argmin.first][argmin.second] = pair.first & FP_MASK;
				heavy_value [argmin.first][argmin.second] = new_val;
				return;
			}

			/** Not a delayed one. Just insert into the light part. */
			auto LightFPs = getLightFPs (pair.first);
			for (int i = 0; i < SKETCH_NUMBER; ++i) {
				int j = LightFPs [i] % SKETCH_WIDTH;
				sketch [i][j] = protectedPlus (sketch [i][j], -1);
			}
			int16_t ret = calculate (pair.first);
			if (ret > BAR) {
				alert (quin, current_time);
				/** Remove the effect of the stream. */
				for (int i = 0; i < SKETCH_NUMBER; ++i) {
					int j = LightFPs [i] % SKETCH_WIDTH;
					sketch [i][j] = protectedPlus (sketch [i][j], (int16_t) -ret);
				}
			}
			return;
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
	const char input_name [] = "../../130000_delay.dat";
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