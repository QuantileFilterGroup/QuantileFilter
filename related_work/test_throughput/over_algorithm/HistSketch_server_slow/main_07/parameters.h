#ifndef _PARAM_H_
#define _PARAM_H_

#define CHARKEY_LEN 13
#define DATA_T_SIZE 15

// hash table
// #define SLOT_NUM ((1 << 11) + (1 << 12) + (1 << 13))*2
#define SLOT_NUM (7 << 12)
#define EVICT_THRESHOLD 1
#define CONTROL_PLANE_PRO 0

// bloom filter
#define BLOOM_SIZE (1 << 21)
#define BLOOM_HASH_NUM 11

// histogram
#define BUCKET_WIDTH 50
// #define BUCKET_NUM 20
#define BUCKET_NUM 2
// #define H_BUCKET_NUM 8
#define H_BUCKET_NUM 2

// cm for histogram
#define CM_DEPTH_HIST 4
// #define CM_WIDTH_HIST (1 << 15)*2
#define CM_WIDTH_HIST (1 << 16)

// metrics
#define POINT_ARETHRESHOLD 0.01
#define HISTOGRAM_ARETHRESHOLD 0.01
#define HC_SIZE 50

// QuantileFilter: Problem Formulation
#define PERCENTAGE 0.95
#define THRESHOLD 300000 			/** Unit: microsecond  */
#define DELTA 30					/** Alert if sum exceeds DELTA * (SCALE + ASCEND_PROB + 1) */

/*************************************/
/****** Do not edit this part! *******/
#define HIT 0
#define HIT_EVICT 1
#define MISS_EVICT 2
#define MISS_INSERT 3

#define HIST_HIT 0
#define HIST_EVICT 1

#define SOLVED

#endif