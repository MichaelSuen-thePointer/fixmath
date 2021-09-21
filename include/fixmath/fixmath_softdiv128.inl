/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

// intentionally omit header guard
// DO NOT MANULLY INCLUDE THIS FILE

namespace fixmath {

// reference from llvm compiler-rt
// a fast 128bit / 64bit algorithm
inline uint64_t _softudiv128(uint64_t u1, uint64_t u0, uint64_t v, uint64_t* r) {
	const unsigned n_udword_bits = 64;
	const uint64_t b = (1ULL << (n_udword_bits / 2)); // Number base (32 bits)
	uint64_t un1, un0;                                // Norm. dividend LSD's
	uint64_t vn1, vn0;                                // Norm. divisor digits
	uint64_t q1, q0;                                  // Quotient digits
	uint64_t un64, un21, un10;                        // Dividend digit pairs
	uint64_t rhat;                                    // A remainder
	int32_t s;                                        // Shift amount for normalization

	s = _fm_clzll(v);
	if (s > 0) {
		// Normalize the divisor.
		v = v << s;
		un64 = (u1 << s) | (u0 >> (n_udword_bits - s));
		un10 = u0 << s; // Shift dividend left
	} else {
		// Avoid undefined behavior of (u0 >> 64).
		un64 = u1;
		un10 = u0;
	}

	// Break divisor up into two 32-bit digits.
	vn1 = v >> (n_udword_bits / 2);
	vn0 = v & 0xFFFF'FFFF;

	// Break right half of dividend into two digits.
	un1 = un10 >> (n_udword_bits / 2);
	un0 = un10 & 0xFFFF'FFFF;

	// Compute the first quotient digit, q1.
	q1 = un64 / vn1;
	rhat = un64 - q1 * vn1;

	// q1 has at most error 2. No more than 2 iterations.
	while (q1 >= b || q1 * vn0 > b * rhat + un1) {
		q1 = q1 - 1;
		rhat = rhat + vn1;
		if (rhat >= b)
		break;
	}

	un21 = un64 * b + un1 - q1 * v;

	// Compute the second quotient digit.
	q0 = un21 / vn1;
	rhat = un21 - q0 * vn1;

	// q0 has at most error 2. No more than 2 iterations.
	while (q0 >= b || q0 * vn0 > b * rhat + un0) {
		q0 = q0 - 1;
		rhat = rhat + vn1;
		if (rhat >= b)
		break;
	}

	*r = (un21 * b + un0 - q0 * v) >> s;
	return q1 * b + q0;
}

} // namespace fixmath
