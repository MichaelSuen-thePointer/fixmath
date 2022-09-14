/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

// intentionally omit header guard
// DO NOT MANULLY INCLUDE THIS FILE

namespace fixmath {

template<class T, class U>
constexpr T _fm_checked_add(T a, T b, U& overflow) {
#if FIXMATH_LINUX
	T r;
	overflow = __builtin_add_overflow(a, b, &r);
	return r;
#else
	if constexpr (::std::is_unsigned<T>::value) {
		a += b;
		overflow = a < b;
		return a;
	} else {
		using UT = ::std::make_unsigned_t<T>;
		UT _a = static_cast<UT>(a);
		UT _b = static_cast<UT>(b);
		UT _r = _a + _b;
		overflow = !!((~(_a ^ _b) & (_a ^ _r)) >> (sizeof(UT) * CHAR_BIT - 1));
		return _r;
	}
#endif
}

template<class T, class U>
constexpr T _fm_checked_sub(T a, T b, U& overflow) {
#if FIXMATH_LINUX
	T r;
	overflow = __builtin_sub_overflow(a, b, &r);
	return r;
#else
	if constexpr (::std::is_unsigned_v<T>) {
		overflow = a < b;
		return a - b;
	} else {
		using UT = ::std::make_unsigned_t<T>;
		UT _a = static_cast<UT>(a);
		UT _b = static_cast<UT>(0) - static_cast<UT>(b);
		UT _r = _a + _b;
		overflow = !!((~(_a ^ _b) & (_a ^ _r)) >> (sizeof(UT) * CHAR_BIT - 1));
		return _r;
	}
#endif
}

constexpr void _fm_neg128(uint64_t& hi, uint64_t& lo) {
#if FIXMATH_LINUX && FIXMATH_64BIT
	__uint128_t a = hi;
	a <<= 64;
	a |= lo;
	a = 0 - a;
	hi = a >> 64;
	lo = a;
#else
	hi = lo ? ~hi : 0 - hi;
	lo = 0 - lo;
#endif
}

constexpr void _fm_neg128(int64_t& hi, int64_t& lo) {
	auto uhi = static_cast<uint64_t>(hi);
	auto ulo = static_cast<uint64_t>(lo);
	_fm_neg128(uhi, ulo);
	hi = static_cast<int64_t>(uhi);
	lo = static_cast<int64_t>(ulo);
}

constexpr void _fm_inc128(uint64_t& hi, uint64_t& lo) {
#if FIXMATH_LINUX && FIXMATH_64BIT
	__uint128_t a = hi;
	a <<= 64;
	a |= lo;
	a += 1;
	hi = a >> 64;
	lo = a;
#else
	lo += 1;
	hi += (lo == 0);
#endif
}

constexpr void _fm_inc128(int64_t& hi, int64_t& lo) {
	auto uhi = static_cast<uint64_t>(hi);
	auto ulo = static_cast<uint64_t>(lo);
	_fm_inc128(uhi, ulo);
	hi = static_cast<int64_t>(uhi);
	lo = static_cast<int64_t>(ulo);
}

constexpr void _fm_add128(int64_t& hi, int64_t& lo, int64_t v) {
#if FIXMATH_LINUX && FIXMATH_64BIT
	__uint128_t a = hi;
	a <<= 64;
	a |= static_cast<uint64_t>(lo);
	a += v;
	hi = a >> 64;
	lo = a;
#else
	uint64_t ulo = static_cast<uint64_t>(lo);
	uint64_t newlo = ulo + v;
	uint64_t carry = (v < 0) ? (newlo > ulo ? -1 : 0) : (newlo < ulo ? 1 : 0);
	lo = newlo;
	hi += carry;
#endif
}

} //namespace fixmath
