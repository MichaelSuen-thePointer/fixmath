/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

// intentionally omit header guard
// DO NOT MANULLY INCLUDE THIS FILE

namespace fixmath {

template <FixedPolicy policy, ::std::size_t N>
fixed<policy> _fm_horner_generic(typename fixed<policy>::raw_t x, const typename fixed<policy>::raw_t (*coefficients)[N]) {
	using fixed = fixed<policy>;
	static_assert(N > 0);

	const fixed fixed_x = fixed::from_raw(x);
	fixed result = fixed::from_raw((*coefficients)[0]);
	for (::std::size_t i = 1; i < N; ++i) {
		result = result * fixed_x + fixed::from_raw((*coefficients)[i]);
	}
	return result;
}

template <FixedPolicy policy, ::std::size_t N>
fixed<policy> _fm_horner_fast128(typename fixed<policy>::raw_t x, const typename fixed<policy>::raw_t (*coefficients)[N]) {
	using fixed = fixed<policy>;
	using raw_t = typename fixed::raw_t;
	static_assert(sizeof(raw_t) == sizeof(int64_t));
	static_assert(N > 0);

	// The caller must prove that each normalized result and following addition fits raw_t.
	raw_t result = (*coefficients)[0];
	for (::std::size_t i = 1; i < N; ++i) {
		raw_t product_high = 0;
		const raw_t product_low = _fm_mul128(result, x, product_high);
		raw_t normalized_high = 0;
		result = _fm_div2n_round<policy, fixed::FRACTION_BITS>(product_high, product_low, normalized_high);
		FIXMATH_ASSERT(normalized_high == (result >> (fixed::ALL_BITS - 1)), "normalized Horner product must fit raw_t");
		(void)normalized_high;
		result += (*coefficients)[i];
	}
	return fixed::from_raw(result);
}

template <FixedPolicy policy, ::std::size_t N>
fixed<policy> _fm_horner_fast64(typename fixed<policy>::raw_t x, const typename fixed<policy>::raw_t (*coefficients)[N]) {
	using fixed = fixed<policy>;
	using raw_t = typename fixed::raw_t;
	static_assert(fixed::FRACTION_BITS == 32);
	static_assert(sizeof(raw_t) == sizeof(int64_t));
	static_assert(N > 0);

	// Offline range analysis must prove that each raw multiply and normalized addition fits raw_t.
	raw_t result = (*coefficients)[0];
	for (::std::size_t i = 1; i < N; ++i) {
		const raw_t product = result * x;
		result = _fm_div2n_round<policy, fixed::FRACTION_BITS>(product);
		result += (*coefficients)[i];
	}
	return fixed::from_raw(result);
}

template <FixedPolicy policy>
	requires(fixed<policy>::FRACTION_BITS == 32)
fixed<policy> _fm_sincos(fixed<policy> a, bool cosine) {
	using fixed = fixed<policy>;
	using raw_t = typename fixed::raw_t;
	// See docs/internals/function-approximations.md for this Q32.32 candidate.
	const static raw_t SIN_COEFFICIENTS[] = {
		11654LL, -852064LL, 35791363LL, -715827879LL, 4294967296LL,
	};
	const static raw_t COS_COEFFICIENTS[] = {
		104756LL, -5964319LL, 178956784LL, -2147483636LL, 4294967296LL,
	};

	if (a > fixed::quarter_pi()) {
		a = fixed::half_pi() - a;
		cosine = !cosine;
	}
	const fixed square = a * a;
	if (cosine) {
		return _fm_horner_fast64<policy>(square.raw(), &COS_COEFFICIENTS);
	}
	return a * _fm_horner_fast64<policy>(square.raw(), &SIN_COEFFICIENTS);
}

template <FixedPolicy policy>
	requires(fixed<policy>::FRACTION_BITS == 32)
fixed<policy> sin(fixed<policy> a) {
	using fixed = fixed<policy>;
	using raw_t = typename fixed::raw_t;
	if constexpr (policy::strict_mode) {
		if (FIXMATH_UNLIKELY(a.is_nan() || a.is_inf())) {
			return fixed::nan();
		}
	}

	const raw_t two_pi = fixed::two_pi().raw();
	const raw_t pi = fixed::pi().raw();
	const raw_t half_pi = fixed::half_pi().raw();
	raw_t reduced = a.raw() % two_pi;
	bool negate = reduced < 0;
	if (negate) {
		reduced = -reduced;
	}
	if (reduced > pi) {
		reduced = two_pi - reduced;
		negate = !negate;
	}
	if (reduced > half_pi) {
		reduced = pi - reduced;
	}

	const fixed result = _fm_sincos(fixed::from_raw(reduced), false);
	return negate ? -result : result;
}

template <FixedPolicy policy>
	requires(fixed<policy>::FRACTION_BITS == 32)
fixed<policy> cos(fixed<policy> a) {
	using fixed = fixed<policy>;
	using raw_t = typename fixed::raw_t;
	if constexpr (policy::strict_mode) {
		if (FIXMATH_UNLIKELY(a.is_nan() || a.is_inf())) {
			return fixed::nan();
		}
	}

	const raw_t two_pi = fixed::two_pi().raw();
	const raw_t pi = fixed::pi().raw();
	const raw_t half_pi = fixed::half_pi().raw();
	raw_t reduced = a.raw() % two_pi;
	if (reduced < 0) {
		reduced = -reduced;
	}
	if (reduced > pi) {
		reduced = two_pi - reduced;
	}
	bool negate = reduced > half_pi;
	if (negate) {
		reduced = pi - reduced;
	}

	const fixed result = _fm_sincos(fixed::from_raw(reduced), true);
	return negate ? -result : result;
}

template <FixedPolicy policy>
constexpr fixed<policy> sqrt(fixed<policy> a) {
	using fixed = fixed<policy>;
	using raw_t = typename fixed::raw_t;
	using uraw_t = typename fixed::uraw_t;
	if constexpr (policy::strict_mode) {
		if (FIXMATH_UNLIKELY(a.is_nan())) {
			return fixed::nan();
		}
		if (FIXMATH_UNLIKELY(a.is_inf() && a.raw() > 0)) {
			return fixed::inf();
		}
		if (FIXMATH_UNLIKELY(a.raw() < 0)) {
			FIXMATH_ERROR("sqrt(<0)");
			return fixed::nan();
		}
	}
	if constexpr (policy::saturation_mode) {
		if (FIXMATH_UNLIKELY(a.raw() < 0)) {
			FIXMATH_ERROR("sqrt(<0)");
			return fixed::min_sat();
		}
	}
	if (a.raw() == 0) {
		return fixed::from_raw(raw_t{0});
	}
	uraw_t value = a.uraw();
	if constexpr (fixed::FRACTION_BITS & 1) {
		value <<= 1;
	}
	uraw_t root = 0;
	uraw_t remainder = 0;
	int start_i = 0;
	if constexpr (sizeof(uraw_t) <= sizeof(uint32_t)) {
		start_i = (_fm_clz(static_cast<uint32_t>(value)) - (sizeof(uint32_t) * CHAR_BIT - fixed::ALL_BITS)) >> 1;
	} else {
		start_i = (_fm_clz(static_cast<uint64_t>(value)) - (sizeof(uint64_t) * CHAR_BIT - fixed::ALL_BITS)) >> 1;
	}
	value <<= (start_i << 1);
	for (raw_t i = start_i; i < fixed::ALL_BITS / 2; ++i) {
		root <<= 1;
		remainder = (remainder << 2) | (value >> (fixed::ALL_BITS - 2));
		uraw_t tester = (root << 1) | 1;
		if (tester <= remainder) {
			root |= 1;
			remainder -= tester;
		}
		value <<= 2;
	}
	for (raw_t i = 0; i < fixed::FRACTION_BITS / 2; ++i) {
		root <<= 1;
		remainder <<= 2;
		uraw_t tester = (root << 1) | 1;
		if (tester <= remainder) {
			root |= 1;
			remainder -= tester;
		}
	}
	uraw_t result = root;
	if constexpr (policy::rounding) {
		root <<= 1;
		remainder <<= 2;
		uraw_t tester = (root << 1) | 1;
		if (tester <= remainder) {
			if (remainder == tester) {
				result += result & 1;
			} else {
				result += 1;
			}
		}
	}
	return fixed::from_raw(result);
}

} // namespace fixmath
