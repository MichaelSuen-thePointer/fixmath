/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

// intentionally omit header guard
// DO NOT MANULLY INCLUDE THIS FILE

namespace fixmath {

struct _fm_pio4_remainder {
	uint64_t remainder;
	uint64_t quotient;
};

// Divide a nonnegative fixed-point raw magnitude by pi/4. The remainder
// retains one fractional bit per underlying bit; quotient preserves the
// octant information needed by trigonometric range reduction.
template <::std::size_t INPUT_FRACTION_BITS, class uraw_t>
inline _fm_pio4_remainder _fm_rem_pio4(uraw_t magnitude) {
	static_assert(::std::is_integral_v<uraw_t> && ::std::is_unsigned_v<uraw_t>, "uraw_t must be an unsigned integer");
	static_assert(sizeof(uraw_t) == sizeof(uint32_t) || sizeof(uraw_t) == sizeof(uint64_t), "uraw_t must be 32 or 64 bits");
	static_assert(INPUT_FRACTION_BITS < sizeof(uraw_t) * CHAR_BIT, "input fraction bits out of range");
	constexpr ::std::size_t PERIOD_FRACTION_BITS = sizeof(uraw_t) * CHAR_BIT;
	constexpr ::std::size_t SCALE_SHIFT = PERIOD_FRACTION_BITS - INPUT_FRACTION_BITS;
	constexpr uint64_t PIO4 = sizeof(uraw_t) == sizeof(uint64_t) ? 0xc90f'daa2'2168'c235ULL : 0xc90f'daa2ULL;

	const uint64_t magnitude64 = static_cast<uint64_t>(magnitude);
	uint64_t scaled_hi = 0;
	uint64_t scaled_lo = 0;
	if constexpr (SCALE_SHIFT == 0) {
		scaled_lo = magnitude64;
	} else if constexpr (SCALE_SHIFT < 64) {
		scaled_lo = magnitude64 << SCALE_SHIFT;
		scaled_hi = magnitude64 >> (64 - SCALE_SHIFT);
	} else {
		scaled_hi = magnitude64 << (SCALE_SHIFT - 64);
	}

	uint64_t remainder = 0;
	uint64_t quotient = 0;
	if (scaled_hi == 0) {
		quotient = scaled_lo / PIO4;
		remainder = scaled_lo % PIO4;
	} else {
		quotient = _fm_udiv128(scaled_hi, scaled_lo, PIO4, remainder);
	}
	return {remainder, quotient};
}

template <FixedPolicy policy, ::std::size_t N>
typename fixed<policy>::raw_t _fm_horner_generic(typename fixed<policy>::raw_t x, const typename fixed<policy>::raw_t (*coefficients)[N]) {
	using fixed = fixed<policy>;
	static_assert(N > 0);

	const fixed fixed_x = fixed::from_raw(x);
	fixed result = fixed::from_raw((*coefficients)[0]);
	for (::std::size_t i = 1; i < N; ++i) {
		result = result * fixed_x + fixed::from_raw((*coefficients)[i]);
	}
	return result.raw();
}

template <FixedPolicy policy, ::std::size_t N>
typename fixed<policy>::raw_t _fm_horner_fast128(typename fixed<policy>::raw_t x, const typename fixed<policy>::raw_t (*coefficients)[N]) {
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
	return result;
}

template <FixedPolicy policy, ::std::size_t N>
typename fixed<policy>::raw_t _fm_horner_fast64(typename fixed<policy>::raw_t x, const typename fixed<policy>::raw_t (*coefficients)[N]) {
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
	return result;
}

template <FixedPolicy policy>
	requires(fixed<policy>::FRACTION_BITS == 32)
typename fixed<policy>::raw_t _fm_sincos(typename fixed<policy>::raw_t a, bool cosine) {
	using fixed = fixed<policy>;
	using raw_t = typename fixed::raw_t;
	using uraw_t = typename fixed::uraw_t;
	// See docs/internals/function-approximations.md for this Q32.32 candidate.
	const static raw_t SIN_COEFFICIENTS[] = {
		11654LL, -852064LL, 35791363LL, -715827879LL, 4294967296LL,
	};
	const static raw_t COS_COEFFICIENTS[] = {
		104756LL, -5964319LL, 178956784LL, -2147483636LL, 4294967296LL,
	};

	if (a > fixed::quarter_pi().raw()) {
		a = fixed::half_pi().raw() - a;
		cosine = !cosine;
	}
	const uraw_t a_raw = static_cast<uraw_t>(a);
	const raw_t square = static_cast<raw_t>(_fm_umul64<policy, fixed::FRACTION_BITS>(a_raw, a_raw));
	if (cosine) {
		return _fm_horner_fast64<policy>(square, &COS_COEFFICIENTS);
	}
	const raw_t polynomial = _fm_horner_fast64<policy>(square, &SIN_COEFFICIENTS);
	return static_cast<raw_t>(_fm_umul64<policy, fixed::FRACTION_BITS>(a_raw, static_cast<uraw_t>(polynomial)));
}

template <FixedPolicy policy>
	requires(fixed<policy>::FRACTION_BITS == 32)
fixed<policy> sin(fixed<policy> a) {
	using fixed = fixed<policy>;
	using raw_t = typename fixed::raw_t;
	using uraw_t = typename fixed::uraw_t;
	if constexpr (policy::strict_mode) {
		if (FIXMATH_UNLIKELY(a.is_nan() || a.is_inf())) {
			return fixed::nan();
		}
	}

	const raw_t raw = a.raw();
	const bool negative = raw < 0;
	const uraw_t magnitude = _fm_absraw(raw);
	const auto [remainder64, quotient] = _fm_rem_pio4<fixed::FRACTION_BITS>(magnitude);
	const raw_t remainder = static_cast<raw_t>(_fm_div2n_round<policy, fixed::FRACTION_BITS>(remainder64));
	const uint32_t octant = static_cast<uint32_t>(quotient & 7);
	const raw_t quarter_pi = fixed::quarter_pi().raw();
	const raw_t half_pi = fixed::half_pi().raw();
	raw_t reduced = 0;
	if ((octant & 3) == 0) {
		reduced = remainder;
	} else if ((octant & 3) == 1) {
		reduced = quarter_pi + remainder;
	} else if ((octant & 3) == 2) {
		reduced = half_pi - remainder;
	} else {
		reduced = quarter_pi - remainder;
	}

	const raw_t result = _fm_sincos<policy>(reduced, false);
	const bool negate = negative != (octant >= 4);
	return fixed::from_raw(negate ? -result : result);
}

template <FixedPolicy policy>
	requires(fixed<policy>::FRACTION_BITS == 32)
fixed<policy> cos(fixed<policy> a) {
	using fixed = fixed<policy>;
	using raw_t = typename fixed::raw_t;
	using uraw_t = typename fixed::uraw_t;
	if constexpr (policy::strict_mode) {
		if (FIXMATH_UNLIKELY(a.is_nan() || a.is_inf())) {
			return fixed::nan();
		}
	}

	const raw_t raw = a.raw();
	const uraw_t magnitude = _fm_absraw(raw);
	const auto [remainder64, quotient] = _fm_rem_pio4<fixed::FRACTION_BITS>(magnitude);
	const raw_t remainder = static_cast<raw_t>(_fm_div2n_round<policy, fixed::FRACTION_BITS>(remainder64));
	const uint32_t octant = static_cast<uint32_t>(quotient & 7);
	const raw_t quarter_pi = fixed::quarter_pi().raw();
	const raw_t half_pi = fixed::half_pi().raw();
	raw_t reduced = 0;
	if ((octant & 3) == 0) {
		reduced = remainder;
	} else if ((octant & 3) == 1) {
		reduced = quarter_pi + remainder;
	} else if ((octant & 3) == 2) {
		reduced = half_pi - remainder;
	} else {
		reduced = quarter_pi - remainder;
	}

	const raw_t result = _fm_sincos<policy>(reduced, true);
	const bool negate = octant >= 2 && octant < 6;
	return fixed::from_raw(negate ? -result : result);
}

template <FixedPolicy policy>
	requires(fixed<policy>::FRACTION_BITS == 32)
typename fixed<policy>::raw_t _fm_tan_kernel(typename fixed<policy>::raw_t a, bool retain_guard_bit) {
	using fixed = fixed<policy>;
	using raw_t = typename fixed::raw_t;
	using uraw_t = typename fixed::uraw_t;
	// See docs/internals/function-approximations.md for this Q32.32 candidate.
	const static raw_t TAN_COEFFICIENTS[] = {
		49715989LL, 90993159LL, 232123842LL, 572645510LL, 1431656075LL, 4294967295LL,
	};

	const uraw_t a_raw = static_cast<uraw_t>(a);
	const raw_t square = static_cast<raw_t>(_fm_umul64<policy, fixed::FRACTION_BITS>(a_raw, a_raw));
	const raw_t polynomial = _fm_horner_fast64<policy>(square, &TAN_COEFFICIENTS);
	const raw_t product = a * polynomial;
	if (retain_guard_bit) {
		return _fm_div2n_round<policy, fixed::FRACTION_BITS - 1>(product);
	}
	return _fm_div2n_round<policy, fixed::FRACTION_BITS>(product);
}

template <FixedPolicy policy>
	requires(fixed<policy>::FRACTION_BITS == 32)
typename fixed<policy>::raw_t _fm_cot_residual_kernel(typename fixed<policy>::raw_t a) {
	using fixed = fixed<policy>;
	using raw_t = typename fixed::raw_t;
	using uraw_t = typename fixed::uraw_t;
	// See docs/internals/function-approximations.md for this Q32.32 candidate.
	const static raw_t COT_RESIDUAL_COEFFICIENTS[] = {
		-949077LL,
		-9084519LL,
		-95443945LL,
		-1431655764LL,
	};

	const uraw_t a_raw = static_cast<uraw_t>(a);
	const raw_t square = static_cast<raw_t>(_fm_umul64<policy, fixed::FRACTION_BITS>(a_raw, a_raw));
	const raw_t polynomial = _fm_horner_fast64<policy>(square, &COT_RESIDUAL_COEFFICIENTS);
	return _fm_div2n_round<policy, fixed::FRACTION_BITS>(a * polynomial);
}

template <FixedPolicy policy>
	requires(fixed<policy>::FRACTION_BITS == 32)
fixed<policy> tan(fixed<policy> a) {
	using fixed = fixed<policy>;
	using raw_t = typename fixed::raw_t;
	using uraw_t = typename fixed::uraw_t;
	if constexpr (policy::strict_mode) {
		if (FIXMATH_UNLIKELY(a.is_nan() || a.is_inf())) {
			return fixed::nan();
		}
	}

	const raw_t raw = a.raw();
	const bool negative = raw < 0;
	const uraw_t magnitude = _fm_absraw(raw);
	const auto [remainder64, quotient] = _fm_rem_pio4<fixed::FRACTION_BITS>(magnitude);
	const raw_t remainder = static_cast<raw_t>(_fm_div2n_round<policy, fixed::FRACTION_BITS>(remainder64));
	const uint32_t quadrant = static_cast<uint32_t>(quotient & 3);
	const raw_t quarter_pi = fixed::quarter_pi().raw();
	const raw_t half_pi = fixed::half_pi().raw();
	raw_t reduced = 0;
	if (quadrant == 0) {
		reduced = remainder;
	} else if (quadrant == 1) {
		reduced = quarter_pi + remainder;
	} else if (quadrant == 2) {
		reduced = half_pi - remainder;
	} else {
		reduced = quarter_pi - remainder;
	}
	const bool negate = negative != (quadrant >= 2);

	if (FIXMATH_UNLIKELY(reduced == half_pi)) {
		if constexpr (policy::strict_mode) {
			return negate ? -fixed::inf() : fixed::inf();
		} else if constexpr (policy::saturation_mode) {
			return negate ? fixed::min_sat() : fixed::max_sat();
		} else {
			return fixed::nan();
		}
	}

	constexpr raw_t DIRECT_BOUNDARY = 1975684956LL;
	const raw_t reciprocal_boundary = half_pi - DIRECT_BOUNDARY;
	raw_t result = 0;
	if (reduced <= DIRECT_BOUNDARY) {
		result = _fm_tan_kernel<policy>(reduced, false);
	} else if (reduced == quarter_pi) {
		result = static_cast<raw_t>(fixed::URATIO);
	} else if (reduced <= quarter_pi) {
		const raw_t reflected = quarter_pi - reduced;
		const raw_t tangent = _fm_tan_kernel<policy>(reflected, true);
		constexpr raw_t GUARDED_ONE = raw_t{1} << (fixed::FRACTION_BITS + 1);
		result = (fixed::from_raw(GUARDED_ONE - tangent) / fixed::from_raw(GUARDED_ONE + tangent)).raw();
	} else if (reduced < reciprocal_boundary) {
		const raw_t reflected = reduced - quarter_pi;
		const raw_t tangent = _fm_tan_kernel<policy>(reflected, true);
		constexpr raw_t GUARDED_ONE = raw_t{1} << (fixed::FRACTION_BITS + 1);
		result = (fixed::from_raw(GUARDED_ONE + tangent) / fixed::from_raw(GUARDED_ONE - tangent)).raw();
	} else {
		const raw_t distance = half_pi - reduced;
		const fixed reciprocal = fixed(1) / fixed::from_raw(distance);
		const fixed residual = fixed::from_raw(_fm_cot_residual_kernel<policy>(distance));
		result = (reciprocal + residual).raw();
	}
	const fixed result_magnitude = fixed::from_raw(result);
	if (!negate) {
		return result_magnitude;
	}
	if constexpr (policy::saturation_mode) {
		if (FIXMATH_UNLIKELY(result_magnitude == fixed::max_sat())) {
			return fixed::min_sat();
		}
	}
	return -result_magnitude;
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
