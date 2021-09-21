/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

// intentionally omit header guard
// DO NOT MANULLY INCLUDE THIS FILE

#include "fixmath_softmul128.inl"
#include "fixmath_softdiv128.inl"

namespace fixmath {

#if FIXMATH_LINUX_X64 || FIXMATH_LINUX_ARM64

inline int64_t _fm_mul128(int64_t a, int64_t b, int64_t& rhi) {
	__int128_t r = a;
	r *= b;
	rhi = static_cast<int64_t>(r >> 64);
	return static_cast<int64_t>(r);
}

inline uint64_t _fm_umul128(uint64_t a, uint64_t b, uint64_t& rhi) {
	__uint128_t r = a;
	r *= b;
	rhi = static_cast<uint64_t>(r >> 64);
	return static_cast<uint64_t>(r);
}

#elif FIXMATH_WIN_X64

inline int64_t _fm_mul128(int64_t a, int64_t b, int64_t& rhi) {
	return ::_mul128(a, b, &rhi);
}

inline uint64_t _fm_umul128(uint64_t a, uint64_t b, uint64_t& rhi) {
	return ::_umul128(a, b, &rhi);
}

#else

inline int64_t _fm_mul128(int64_t a, int64_t b, int64_t& rhi) {
	return _softmul128(a, b, rhi);
}

inline uint64_t _fm_umul128(uint64_t a, uint64_t b, uint64_t& rhi) {
	return _softumul128(a, b, rhi);
}

#endif

struct _int128_s {
	int64_t lo;
	int64_t hi;
};

inline _int128_s _fm_div128(int64_t dhi, int64_t dlo, int64_t d, int64_t& rem) {
	uint64_t udhi = static_cast<uint64_t>(dhi);
	uint64_t udlo = static_cast<uint64_t>(dlo);
	uint64_t ud = static_cast<uint64_t>(d);
	if (dhi < 0) {
		_fm_neg128(udhi, udlo);
	}
	if (d < 0) {
		ud = 0 - ud;
	}
	uint64_t urem = 0;
	uint64_t uqhi = 0;
	uint64_t uqlo = 0;
	if (udhi >= ud) {
		uqhi = udhi / ud;
		urem = udhi % ud;
	} else {
		uqhi = 0;
		urem = udhi;
	}
#if FIXMATH_LINUX_X64
	__asm__("divq %[v]"
		: "=a"(uqlo), "=d"(urem)
		: [v] "r"(ud), "a"(udlo), "d"(urem));
#elif FIXMATH_WIN_X64 && FIXMATH_HAS_INTRIN_DIV
	uqlo = ::_udiv128(urem, udlo, ud, &urem);
#else
	uqlo = _softudiv128(urem, udlo, ud, &urem);
#endif
	if ((dhi ^ d) < 0) {
		_fm_neg128(uqhi, uqlo);
	}
	if (dhi < 0) {
		urem = 0 - urem;
	}
	rem = static_cast<int64_t>(urem);
	return { static_cast<int64_t>(uqlo), static_cast<int64_t>(uqhi) };
}

template<class policy, size_t N, class T>
inline T _fm_div2n_round(T a) {
	// 除以2的N次并向偶舍入(如果有开关)
	const int bits = sizeof(a) * 8;
	static_assert(N < bits - (2 - std::is_unsigned<T>::value), "cannot touch sign bit");
	if constexpr (N != 0) {
		if constexpr (policy::rounding) {
			using UT = typename std::make_unsigned<T>::type;
			const UT half = UT(1) << (N - 1);
			const UT frac = UT(a) << (bits - N) >> (bits - N);
			a >>= N;
			bool carry1 = frac > half;
			bool carry2 = (frac == half) && (a & 1);
			a += carry1 | carry2;
		} else {
			a /= (T(1) << N);
		}
	}
	return a;
}

template<class policy, class T>
inline T _fm_div2n_round(T a, uint64_t n) {
	// 除以2的N次并向偶舍入
	const int bits = sizeof(a) * 8;
	(void)bits;
	FIXMATH_ASSERT((n >= 0 && n < bits - (2 - std::is_unsigned<T>::value)), "bug");
	if (n != 0) {
		if constexpr (policy::rounding) {
			using UT = typename std::make_unsigned<T>::type;
			const UT half = UT(1) << (n - 1);
			const UT frac = UT(a) << (bits - n) >> (bits - n);
			a >>= n;
			bool carry1 = frac > half;
			bool carry2 = (frac == half) && (a & 1);
			a += carry1 | carry2;
		} else {
			a /= (T(1) << n);
		}
	}
	return a;
}

template<class policy, size_t N>
inline uint64_t _fm_div2n_round(uint64_t rhi, uint64_t rlo, uint64_t& ohi) {
	static_assert(N > 0 && N < 64, "bug");
	if constexpr (policy::rounding) {
		const uint64_t mask = (uint64_t(1) << N) - 1;
		const uint64_t half = uint64_t(1) << (N - 1);
		// 除以2的幂次并向偶舍入（这里是除以2^32）
		// https://km.netease.com/article/342196
		uint64_t frac = rlo & mask;
		rlo = (rlo >> N) | (rhi << (64 - N));
		rhi >>= N;
		bool carry1 = frac > half;
		bool carry2 = (frac == half) && (rlo & 1);
		rlo = _fm_checked_add(rlo, uint64_t(carry1 | carry2), carry1);
		rhi += carry1;
	} else {
		rlo = (rlo >> N) | (rhi << (64 - N));
		rhi >>= N;
	}
	ohi = rhi;
	return rlo;
}

template<class policy, size_t N>
inline int64_t _fm_div2n_round(int64_t rhi, int64_t rlo, int64_t& ohi) {
	static_assert(N > 0 && N < 64, "bug");
	if constexpr (policy::rounding) {
		const int64_t fracion_mask = int64_t(uint64_t(-1) >> (64 - N));
		const int64_t half = int64_t(1) << (N - 1);
		// 除以2的幂次并向偶舍入（这里是除以2^32）
		// https://km.netease.com/article/342196
		int64_t fraction = rlo & fracion_mask;
		rlo = (static_cast<uint64_t>(rlo) >> N) | (rhi << (64 - N));
		rhi >>= N;
		if (FIXMATH_LIKELY((fraction > half) || ((fraction == half) && (rlo & 1)))) {
			_fm_inc128(rhi, rlo);
		}
	} else {
		const uint64_t fracion_mask = uint64_t(rhi >> 63) >> (64 - N);
		uint64_t c = 0;
		rlo = int64_t(_fm_checked_add(uint64_t(rlo), fracion_mask, c));
		rhi = static_cast<uint64_t>(rhi) + c;
		rlo = (static_cast<uint64_t>(rlo) >> N) | (rhi << (64 - N));
		rhi >>= N;
	}
	ohi = rhi;
	return rlo;
}

inline _int128_s _fm_shl32div(int64_t a, int64_t b, int64_t& rem) {
	uint64_t absa = static_cast<uint64_t>(a);
	uint64_t absb = static_cast<uint64_t>(b);
	if (a < 0) {
		absa = 0 - absa;
	}
	if (b < 0) {
		absb = 0 - absb;
	}
	uint64_t urem = 0;
	uint64_t uqhi = 0;
	uint64_t uqlo = 0;
	uint64_t rs32 = absa >> 32;
	if (rs32 >= absb) {
		uqhi = rs32 / absb;
		urem = rs32 % absb;
	} else {
		uqhi = 0;
		urem = rs32;
	}
#if FIXMATH_LINUX_X64
	__asm__("divq %[v]"
		: "=a"(uqlo), "=d"(urem)
		: [v] "r"(absb), "a"(absa << 32), "d"(urem));
#elif FIXMATH_WIN_X64 && FIXMATH_HAS_INTRIN_DIV
	uqlo = ::_udiv128(urem, absa << 32, absb, &urem);
#else
	uqlo = _softudiv128(urem, absa << 32, absb, &urem);
#endif
	if ((a ^ b) < 0) {
		_fm_neg128(uqhi, uqlo);
	}
	if (a < 0) {
		urem = 0 - urem;
	}
	rem = static_cast<int64_t>(urem);
	return { static_cast<int64_t>(uqlo), static_cast<int64_t>(uqhi) };
}

} // namespace fixmath
