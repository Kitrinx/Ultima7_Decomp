#include "u7/sha256.h"

#include <stdint.h>
#include <string.h>

static const uint32_t ROUND[64] = {
	0x428a2f98u, 0x71374491u, 0xb5c0fbcfu, 0xe9b5dba5u, 0x3956c25bu, 0x59f111f1u,
	0x923f82a4u, 0xab1c5ed5u, 0xd807aa98u, 0x12835b01u, 0x243185beu, 0x550c7dc3u,
	0x72be5d74u, 0x80deb1feu, 0x9bdc06a7u, 0xc19bf174u, 0xe49b69c1u, 0xefbe4786u,
	0x0fc19dc6u, 0x240ca1ccu, 0x2de92c6fu, 0x4a7484aau, 0x5cb0a9dcu, 0x76f988dau,
	0x983e5152u, 0xa831c66du, 0xb00327c8u, 0xbf597fc7u, 0xc6e00bf3u, 0xd5a79147u,
	0x06ca6351u, 0x14292967u, 0x27b70a85u, 0x2e1b2138u, 0x4d2c6dfcu, 0x53380d13u,
	0x650a7354u, 0x766a0abbu, 0x81c2c92eu, 0x92722c85u, 0xa2bfe8a1u, 0xa81a664bu,
	0xc24b8b70u, 0xc76c51a3u, 0xd192e819u, 0xd6990624u, 0xf40e3585u, 0x106aa070u,
	0x19a4c116u, 0x1e376c08u, 0x2748774cu, 0x34b0bcb5u, 0x391c0cb3u, 0x4ed8aa4au,
	0x5b9cca4fu, 0x682e6ff3u, 0x748f82eeu, 0x78a5636fu, 0x84c87814u, 0x8cc70208u,
	0x90befffau, 0xa4506cebu, 0xbef9a3f7u, 0xc67178f2u
};

static uint32_t rotate(uint32_t value, unsigned count) {
	return (value >> count) | (value << (32U - count));
}

static void transform(uint32_t state[8], const uint8_t block[64]) {
	uint32_t w[64];
	for (size_t i = 0; i < 16; ++i) {
		w[i] = (uint32_t) block[i * 4U] << 24 | (uint32_t) block[i * 4U + 1U] << 16 |
				(uint32_t) block[i * 4U + 2U] << 8 | block[i * 4U + 3U];
	}
	for (size_t i = 16; i < 64; ++i) {
		uint32_t s0 = rotate(w[i - 15], 7) ^ rotate(w[i - 15], 18) ^ (w[i - 15] >> 3);
		uint32_t s1 = rotate(w[i - 2], 17) ^ rotate(w[i - 2], 19) ^ (w[i - 2] >> 10);
		w[i] = w[i - 16] + s0 + w[i - 7] + s1;
	}
	uint32_t v[8];
	memcpy(v, state, sizeof v);
	for (size_t i = 0; i < 64; ++i) {
		uint32_t t1 = v[7] + (rotate(v[4], 6) ^ rotate(v[4], 11) ^ rotate(v[4], 25)) +
				((v[4] & v[5]) ^ (~v[4] & v[6])) + ROUND[i] + w[i];
		uint32_t t2 = (rotate(v[0], 2) ^ rotate(v[0], 13) ^ rotate(v[0], 22)) +
				((v[0] & v[1]) ^ (v[0] & v[2]) ^ (v[1] & v[2]));
		memmove(v + 1, v, 7 * sizeof *v);
		v[4] += t1;
		v[0] = t1 + t2;
	}
	for (size_t i = 0; i < 8; ++i) {
		state[i] += v[i];
	}
}

void u7_sha256_hex(const void *data, size_t size, char out[65]) {
	uint32_t state[8] = {
		0x6a09e667u, 0xbb67ae85u, 0x3c6ef372u, 0xa54ff53au,
		0x510e527fu, 0x9b05688cu, 0x1f83d9abu, 0x5be0cd19u
	};
	const uint8_t *bytes = data;
	size_t done = 0;
	for (; done + 64U <= size; done += 64U) {
		transform(state, bytes + done);
	}
	/* The tail, a 1 bit, zeros, and the length in bits, over one or two blocks. */
	uint8_t tail[128] = {0};
	size_t rest = size - done;
	memcpy(tail, bytes + done, rest);
	tail[rest] = 0x80U;
	size_t length = rest < 56U ? 64U : 128U;
	uint64_t bits = (uint64_t) size * 8U;
	for (size_t i = 0; i < 8; ++i) {
		tail[length - 1U - i] = (uint8_t) (bits >> (i * 8U));
	}
	for (size_t at = 0; at < length; at += 64U) {
		transform(state, tail + at);
	}
	static const char DIGITS[] = "0123456789abcdef";
	for (size_t i = 0; i < 32; ++i) {
		uint8_t byte = (uint8_t) (state[i / 4U] >> (24U - (i % 4U) * 8U));
		out[i * 2U] = DIGITS[byte >> 4];
		out[i * 2U + 1U] = DIGITS[byte & 0x0FU];
	}
	out[64] = 0;
}
