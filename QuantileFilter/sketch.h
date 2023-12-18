#ifndef SKETCH_H
#define SKETCH_H

#include <algorithm>
#include <cassert>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <random>
#include <vector>
#include "hash.h"
#include "parameters.h"
#include "quintet.h"

const int16_t SCALE = (uint16_t) (PERCENTAGE / (1 - PERCENTAGE));
const double ASCEND_PROB = PERCENTAGE / (1 - PERCENTAGE) - SCALE;
const int BAR = (int) (DELTA * (SCALE + ASCEND_PROB + 1));

class Sketch {
	private:
		int8_t sketch [SKETCH_NUMBER][SKETCH_WIDTH];
		uint16_t heavy_fp [BLOCK_NUMBER][BLOCK_LENGTH];
		int16_t heavy_value [BLOCK_NUMBER][BLOCK_LENGTH];
		std::mt19937 rnd;
		std::vector <uint32_t> seeds;
		int count_alert;
		FILE *file;
		bool to_alert;
		uint32_t lightFPs [SKETCH_NUMBER + 1];
		int8_t numbers [SKETCH_NUMBER];
	public:
		/**
		 * Return memory the sketch currently uses.
		 * Unit: byte
		*/
		size_t memoryUsage () {
			return sizeof (sketch) + sizeof (heavy_fp) + sizeof (heavy_value) +
				sizeof (rnd) + sizeof (seeds) + sizeof (count_alert) +
				sizeof (file) + sizeof (lightFPs) + sizeof (numbers) +
				seeds.capacity () * sizeof (uint32_t);
		}

		void init (int argc, char **argv, const char *file_name, bool alert_activate) {
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
			if (file != NULL) {
				fclose (file);
				file = NULL;
			}
			file = fopen (file_name, "w");
			assert (file != NULL);
			count_alert = 0;
			to_alert = alert_activate;
			/** Generate SKETCH_NUMBER random seeds as different hash functions, */
			seeds.clear ();
			for (int i = 0; i < SKETCH_NUMBER + 2; ++i) {
				seeds.push_back (rnd ());
			}
			for (int i = 0; i < BLOCK_NUMBER; ++i) {
				for (int j = 0; j < BLOCK_LENGTH; ++j) {
					heavy_value [i][j] = EMPTY_BLOCK;
					heavy_fp [i][j] = 0;
				}
			}
			memset (sketch, 0, sizeof (sketch));
			printf ("Sketch initialization succeed. Memory usage: %llu bytes.\n", memoryUsage ());
		}

		inline uint32_t alignTotalFP (const uint32_t &fp) {
			#ifdef TOTAL_FP_MASK
				return fp & TOTAL_FP_MASK;
			#else
				return fp % TOTAL_FP_DIVISOR;
			#endif
		}

		/**
		 * Return a pair of <finger prints, delay>,
		 * where finger prints are generated with differnt seeds,
		 * and the number of seeds is SKETCH_NUMBER.
		 * Every single seed conrresponds to a single hash function.
		*/
		std::pair <uint32_t, int> getPair (const Quintet &quin) {
			assert (quin.delay >= 0);
			uint32_t long_fp = alignTotalFP (murmur3_32 (quin, seeds [0]));
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
			return (alignTotalFP (fp) >> 16);
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
		void getLightFPs (uint32_t *lightFPs, uint32_t fp) {
			for (int i = 1; i < SKETCH_NUMBER + 2; ++i) {
				lightFPs [i - 1] = murmur3_32_single (fp, seeds [i]);
			}
			return;
		}

		inline uint32_t getLightIndex (const uint32_t &lightFP) {
			#ifdef SKETCH_MASK
				return lightFP & SKETCH_MASK;
			#else
				return lightFP % SKETCH_WIDTH;
			#endif
		}

		/**
		 * Calculate a stream's summary with its finger prints.
		 * In this version, we use Count Sketch, that is, to return
		 * the median of all positions of the stream as the summary.
		*/
		int16_t calculate (const uint32_t &fp) {
			auto pair = findBlock (fp);
			if (pair.first >= 0 && pair.second >= 0) {
				return heavy_value [pair.first][pair.second];
			}
			getLightFPs (lightFPs, fp);
			for (int i = 0; i < SKETCH_NUMBER; ++i) {
				numbers [i] = sketch[i][getLightIndex (lightFPs[i])];
			}
			std::sort (numbers, numbers + SKETCH_NUMBER);
			int16_t ret = (numbers[SKETCH_NUMBER / 2] + numbers[(SKETCH_NUMBER - 1) / 2]) / 2;
			return ret;
		}

		/**
		 * Calculate the full finger print with a block index and
		 * the short 16-bit finger print.
		 */
		uint32_t reviveFullFP (uint32_t index, uint16_t fp) {
			return alignTotalFP ((index << 16) | fp);
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
		 * A protection that a plus operation for 8-bit int won't get overflow.
		*/
		int8_t protectedPlus_8 (int8_t originalValue, int8_t delta) {
				int8_t newValue = (int8_t) (originalValue + delta);
				if (delta < 0 && newValue > originalValue) {
						return MIN_VAL_8;
				}
				if (delta > 0 && newValue < originalValue) {
						return MAX_VAL_8;
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
			getLightFPs (lightFPs, fp);
			if (value > MAX_VAL_8)
				value = MAX_VAL_8;
			if (value < MIN_VAL_8)
				value = MIN_VAL_8;
			int16_t sign = (lightFPs [SKETCH_NUMBER] & 1) ? -1 : 1;
			for (int i = 0; i < SKETCH_NUMBER; ++i) {
				uint32_t j = getLightIndex (lightFPs [i]);
				sketch [i][j] = protectedPlus_8 (sketch [i][j], (int8_t) (sign * value));
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
					if (to_alert)
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
				getLightFPs (lightFPs, pair.first);
				int16_t sign = (lightFPs [SKETCH_NUMBER] & 1) ? -1 : 1;
				int16_t new_val = val;
				new_val = protectedPlus (new_val, (int16_t) (sign * SCALE));
				if (rnd () <= ASCEND_PROB * rnd.max ()) {
					new_val = protectedPlus (new_val, sign);
				}
				/** Value of the stream is large enough for alerting. */
				if (sign * new_val > BAR) {
					/** Remove the effect of the stream in light part. */
					if (to_alert)
						alert (quin, current_time);
					for (int i = 0; i < SKETCH_NUMBER; ++i) {
						uint32_t j = getLightIndex (lightFPs [i]);
						sketch [i][j] = protectedPlus_8 (sketch [i][j], (int8_t) -val);
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
				if (tmp_val >= sign * new_val) {
					int16_t delta = protectedPlus (new_val, (int16_t) -val);
					if (delta > MAX_VAL_8)
						delta = MAX_VAL_8;
					if (delta < MIN_VAL_8)
						delta = MIN_VAL_8;
					getLightFPs (lightFPs, pair.first);
					for (int i = 0; i < SKETCH_NUMBER; ++i) {
						uint32_t j = getLightIndex (lightFPs [i]);
						sketch [i][j] = protectedPlus_8 (sketch [i][j], (int8_t) delta);
					}
					return;
				}
				/** Replacing. Add the old heavy one to light part. */
				getLightFPs (lightFPs, pair.first);
				for (int i = 0; i < SKETCH_NUMBER; ++i) {
					uint32_t j = getLightIndex (lightFPs [i]);
					sketch [i][j] = protectedPlus_8 (sketch [i][j], (int8_t) -val);
				}
				if (tmp_val != EMPTY_BLOCK) {
					insertLight (reviveFullFP (argmin.first, heavy_fp [argmin.first][argmin.second]),
						tmp_val);
				}

				heavy_fp [argmin.first][argmin.second] = pair.first & FP_MASK;
				heavy_value [argmin.first][argmin.second] = (int16_t) (sign * new_val);
				return;
			}

			/** Not a delayed one. Just insert into the light part. */
			getLightFPs (lightFPs, pair.first);
			int16_t sign = (lightFPs [SKETCH_NUMBER] & 1) ? -1 : 1;
			for (int i = 0; i < SKETCH_NUMBER; ++i) {
				uint32_t j = getLightIndex (lightFPs [i]);
				sketch [i][j] = protectedPlus_8 (sketch [i][j], (int8_t) -sign);
			}
			int16_t ret = calculate (pair.first);
			if (ret * sign > BAR) {
				if (to_alert)
					alert (quin, current_time);
				/** Remove the effect of the stream. */
				for (int i = 0; i < SKETCH_NUMBER; ++i) {
					uint32_t j = getLightIndex (lightFPs [i]);
					sketch [i][j] = protectedPlus_8 (sketch [i][j], (int8_t) -ret);
				}
			}
			return;
		}

		void finish (double mops) {
			if (to_alert) {
				printf ("Times of alerting: %d\n", count_alert);
				printf ("Final memory usage: %llu bytes.", memoryUsage ());
			}
			fclose (file);
			file = NULL;
			return;
		}
};

#endif