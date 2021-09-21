/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

// intentionally omit header guard
// DO NOT MANULLY INCLUDE THIS FILE

namespace fixmath {

#if FIXMATH_LINUX
inline int _fm_clzll(uint64_t x) {
	if (x == 0) {
		return 64;
	}
	return __builtin_clzll(x);
}
#elif FIXMATH_WIN && FIXMATH_64BIT
inline int _fm_clzll(uint64_t value) {
	unsigned long leading_zero = 0;
	if (_BitScanReverse64(&leading_zero, value)) {
		return static_cast<int>(63 - leading_zero);
	} else {
		return 64;
	}
}
#elif FIXMATH_WIN && FIXMATH_32BIT
inline int _fm_clzll(uint64_t value) {
	unsigned long leading_zero = 0;
	if (_BitScanReverse(&leading_zero, (unsigned long)(value >> 32))) {
		return static_cast<int>(31 - leading_zero);
	} else if (_BitScanReverse(&leading_zero, (unsigned long)value)) {
		return static_cast<int>(63 - leading_zero);
	} else {
		return 64;
	}
}
#else
inline int _fm_clzll(uint64_t x) {
	int result = 0;
	if (x == 0) return 64;
	while (!(x & 0xF000000000000000ULL)) { result += 4; x <<= 4; }
	while (!(x & 0x8000000000000000ULL)) { result += 1; x <<= 1; }
	return result;
}
#endif

#if FIXMATH_LINUX
inline int _fm_clz(uint32_t x) {
	if (x == 0) {
		return 32;
	}
	return __builtin_clz(x);
}
#elif FIXMATH_WIN
inline int _fm_clz(uint32_t value) {
	unsigned long leading_zero = 0;
	if (_BitScanReverse(&leading_zero, value)) {
		return static_cast<int>(31 - leading_zero);
	} else {
		// Same remarks as above
		return 32;
	}
}
#else
inline int _fm_clz(uint32_t x) {
	int result = 0;
	if (x == 0) return 32;
	while (!(x & 0xF0000000)) { result += 4; x <<= 4; }
	while (!(x & 0x80000000)) { result += 1; x <<= 1; }
	return result;
}
#endif

} // namespace fixmath
