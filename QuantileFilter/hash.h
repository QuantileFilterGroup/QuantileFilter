#ifndef HASH_H
#define HASH_H

#include <cstdint>
#include "quintet.h"

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

#endif