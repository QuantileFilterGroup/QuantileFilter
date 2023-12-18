#ifndef PARAMETERS_H
#define PARAMETERS_H

#define SKETCH_WIDTH (1 << 13)
#if (SKETCH_WIDTH == (SKETCH_WIDTH & -SKETCH_WIDTH))
	#define SKETCH_MASK (SKETCH_WIDTH - 1)
#endif
#define SKETCH_NUMBER 3
#define PERCENTAGE 0.95
#define THRESHOLD 300000 			/** Unit: microsecond  */
#define DELTA 30					/** Alert if sum exceeds DELTA * (SCALE + ASCEND_PROB + 1) */
#define BLOCK_NUMBER_BITS 11
#define BLOCK_NUMBER (1ll << BLOCK_NUMBER_BITS << 1)
#define BLOCK_LENGTH 6
#define FP_MASK ((1 << 16) - 1)
#if (BLOCK_NUMBER == (BLOCK_NUMBER & -BLOCK_NUMBER))
	#define TOTAL_FP_MASK ((BLOCK_NUMBER << 16) - 1)
#else
	#define TOTAL_FP_DIVISOR (BLOCK_NUMBER << 16)
#endif
#define EMPTY_BLOCK ((int16_t) 0x8000)
#define MIN_VAL ((int16_t) 0x8001)
#define MAX_VAL ((int16_t) 0x7FFF)
#define MIN_VAL_8 ((int8_t) 0x81)
#define MAX_VAL_8 ((int8_t) 0x7F)

#endif